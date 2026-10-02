; ****************************************************************************
; layout.inc - the kernel's layout: where state lives (docs/conventions.md, "Where state lives").  Every fixed
; address the kernel uses is defined here, and nowhere else.
;
;   A task's zero page:  $00-$01 its bank registers, $02-$21 r0-r15 (the call registers), $22-$7F the
;                        program's, $80-$FF the OS's (TK_*, K_*, KC_*: below)
;   A task's OS area:    $0200-$03FF (TA_*: below)
;   The kernel task:     task 0; its program zero page ($22-$7F: K0_*) and its RAM from $0400 (the K_* tables)

.include "hw.inc"

ABI_VERSION     = 1             ; The jump table's version (SYSINFO): appended to, never changed
KERNEL_TASK     = 0
INIT_TASK       = 1
TASKS           = 16

; ---- The call registers, r0-r15 ($02-$21): arguments and results of calls; any call may change them
r0              = $02
r1              = $04
r2              = $06
r3              = $08
r4              = $0A
r5              = $0C
r6              = $0E
r7              = $10
r8              = $12
r9              = $14
r10             = $16
r11             = $18
r12             = $1A
r13             = $1C
r14             = $1E
r15             = $20
PROG_ZP         = $22           ; The program's zero page: $22-$7F, never touched by the OS
PROG_ZP_END     = $80

; ---- A task's OS zero page ($80-$FF): the kernel's, in every task.  TK_*: the task's own state (the
; scheduler reads other tasks' by quick looks: kdefs.inc); KC_*: kcopy's; K_*: the call stubs' scratch, in the
; calling task.  $B0-$FF are reserved for the kernel's growth.
TK_SP           = $80           ; Its stack pointer, while it isn't running (its frame is above it).  While it
                                ;   runs, the IRQ dispatcher's (an interrupt saves S here, and restores it)
TK_STATE        = $81           ; The state of the context on top of its stack (ST_*)
TK_FLAGS        = $82           ; TF_*
TK_PREEMPT      = $83           ; No task switch while it's not 0: PREEMPT_OFF's nesting, and the scheduler's
                                ;   own while it picks (the kernel task's is always at least 1)
TK_DUE          = $84           ; A task switch came due while preemption was off
TK_BUSY         = $85           ; Serving a call (SCALL), or a driver still starting: calls to it wait; or
                                ;   BUSY_NOSERVE: it serves no calls (no serve entry)
TK_BLOCKEDON    = $86           ; ST_BLOCKED: the busy task it's waiting to call
TK_INCALLER     = $88           ; SCALL: the calling task, put here by the caller (a quick look)
TK_INA          = $89           ; SCALL: registers crossing the switch
TK_INX          = $8A
TK_PICKS        = $8B           ; The scheduler's count of tasks to look at (in the task being switched out:
                                ;   never the call stubs' scratch, which a switch can come in the middle of)
KC_PTR          = $90           ; (2) kcopy: this task's end of the copy
KC_PARTNER      = $92           ;   the other task
K_A             = $94           ; The call stubs' scratch (in the calling task).  K_A and K_X are SCALL's
K_X             = $95           ;   own: an SCALL or KCALL changes them
K_Y             = $96
K_TMP           = $99
K_TMP2          = $9A
K_PTR           = $9B           ; (2)
K_PTR2          = $9D           ; (2)
K_CNT           = $9F           ; (2)
K_TASK          = $A1           ; A task being worked on
OS_ZP_END       = $A2           ; (Checked: the OS zero page's end, at most $100)
.assert     OS_ZP_END <= $100, error, "The OS zero page runs past $FF"

; Task states (TK_STATE): the state of the context on top of the task's stack.  Only ST_READY runs (and
; ST_SLEEP once its time has come, ST_BLOCKED once its task is free: the scheduler makes them ST_READY)
ST_FREE         = 0             ; Not in use
ST_READY        = 1             ; Runnable
ST_WAIT         = 2             ; Waiting for a wake (WAKE, a wait mask)
ST_CALL         = 3             ; Calling another task (SCALL): runs again when the call returns
ST_IDLE         = 4             ; A driver between requests: runs only for its calls and interrupts
ST_NEW          = 5             ; Being set up
ST_SLEEP        = 6             ; Sleeping till a tick count (K_WAKE_LO/HI, the kernel task's)
ST_BLOCKED      = 7             ; Waiting to SCALL a busy task (TK_BLOCKEDON)
TF_DRIVER       = $01           ; TK_FLAGS: a driver
BUSY_NOSERVE    = $80           ; TK_BUSY: no serve entry (a program): a call to it is E_NODEV
IRQ_RESCHED     = $80           ; An irq entry's answer: a task switch, please (it woke a task)

; A switched-out task's frame, on its own stack, top first (TK_SP points just below it)
;   U, Y, W, X, A, P, PCL, PCH          (as the IRQ path and YIELD build it; SCHED_RESUME unwinds it)
FRAME_SIZE      = 8
FRAME_SP        = $FF - FRAME_SIZE  ; A new task's TK_SP
FR_U            = 1             ; (Offsets from TK_SP + $0100)
FR_Y            = 2
FR_W            = 3
FR_X            = 4
FR_A            = 5
FR_P            = 6
FR_PCL          = 7
FR_PCH          = 8

; ---- A task's OS area ($0200-$03FF)
TA_INBOX        = $0200         ; (32) A server's request block (docs/conventions.md)
TA_IRQVEC       = $0220         ; (2) Its interrupt handler (for the lines it owns)
TA_SERVEVEC     = $0222         ; (2) Its serve entry (SCALL)
TA_ENTRY        = $0224         ; (2) Where it starts: main, or a driver's init
TA_MODBANK      = $0226         ; Its module's paged ROM bank (in place), or $FF (a RAM program)
TA_NAME         = $0230         ; (16) Its name, zero-terminated
TA_OWNERS       = $0240         ; (16) Each IRQ line's owner task ($FF: none): a copy in every task, so the IRQ
                                ;   dispatcher reads it where it is (IRQ_OWN writes them all)
TA_ARGS         = $0300         ; (256) Its arguments, zero-terminated (SPAWN copies them here; r0 at the start)
TA_ARGS_MAX     = 255

; ---- The kernel task's program zero page (task 0's $22-$7F)
K0_TICKS        = $22           ; (2) The tick count (TICK_HZ a second)
K0_CLOCK        = $24           ; (4) The clock: seconds since 2000-01-01 00:00:00
K0_CLOCKSUB     = $28           ;   ticks left in this second
K0_FNVEC        = $2D           ; (2) KCALL: the routine to run (put here by the caller)
K0_MODMASK      = $2F           ; (2) The RAM modules installed (bit = module)
K0_MODCOUNT     = $31           ;   and how many
K0_PTR          = $32           ; (2) KCALL routines' pointers and scratch
K0_PTR2         = $34           ; (2)
K0_TMP          = $36
K0_TMP2         = $37
K0_TMP3         = $38
K0_NEW          = $3A           ; A task being set up (task.s) ...
K0_NEWBANK      = $3B           ;   its module's bank ...
K0_NEWTYPE      = $3C           ;   and type (HT_*)
K0_END          = $3D
.assert     K0_END <= PROG_ZP_END, error, "The kernel task's zero page runs past $7F"

TICK_HZ         = 200
TICK_LATCH      = (CLK_CPS + TICK_HZ / 2) / TICK_HZ - 2     ; Timer 1 free-running: latch + 2 cycles a period
.assert     TICK_LATCH < $10000, error, "The tick doesn't fit VIA timer 1"

; ---- The kernel task's tables (task 0's RAM, from $0400)
K_WAKE_LO       = $0400         ; (16) Each sleeping task's wake time (a tick count): low bytes ...
K_IRQ_STRAY     = $0410         ; (16) Interrupts on lines nobody owns, counted
K_PARENT        = $0420         ; (16) Each task's parent ($FF: none)
K_EXIT_STATE    = $0430         ; (16) A task's exit record: EX_NONE, EX_HELD (not yet waited for)
K_EXIT_CODE     = $0440         ; (16)   its code
K_EXIT_PARENT   = $0450         ; (16)   the parent it's for
K_TASK_BANK     = $0460         ; (16) Each task's module bank ($FF: none)
K_TASK_TYPE     = $0470         ; (16) Each task's module type (HT_*), 0: none
K_WAKE_HI       = $0480         ; (16)   ... and high bytes
K_EXIT_MSG      = $0500         ; (16 x 32) The exit records' messages
EXIT_MSG_MAX    = 31            ;   (31 characters and the 0)
EX_NONE         = 0
EX_HELD         = 1

; ---- The executable and module header, HYX2 (docs/conventions.md): 48 bytes, programs and modules alike
HX_MAGIC        = 0             ; "HYX2"
HX_HSIZE        = 4             ; The header's size (48)
HX_TYPE         = 5             ; HT_*
HX_FLAGS        = 6             ; HF_*
HX_ABI          = 7             ; The ABI version it was built for
HX_LOAD         = 8             ; (2) Load address: $A000 in place, $0800 a RAM program
HX_LENGTH       = 10            ; (2) The image's length
HX_DATA_LOAD    = 12            ; (2) Its initialised data: where it is in the image ...
HX_DATA_RUN     = 14            ; (2)   where it runs (RAM) ...
HX_DATA_LEN     = 16            ; (2)   and its length
HX_BSS          = 18            ; (2) Its BSS: where ...
HX_BSS_LEN      = 20            ; (2)   and how long (cleared)
HX_TOP          = 22            ; (2) The top of the RAM it uses
HX_MAIN         = 24            ; (2) Entry: main (a program), init (a driver)
HX_SERVE        = 26            ; (2) Entry: serve (a driver)
HX_IRQ          = 28            ; (2) Entry: irq (a driver)
HX_STOP         = 30            ; (2) Entry: stop (a driver)
HX_DEVICE       = 32            ; Its device letter (a driver)
HX_BANKS        = 33            ; Banks it spans (in place)
HX_VERSION      = 34            ; (2) Its version
HX_NAME         = 36            ; (12) Its name, zero-terminated (11 characters at most)
HX_SIZE         = 48
HX_NAME_MAX     = 11
HT_PROGRAM      = 1
HT_DRIVER       = 2
HT_LIBRARY      = 3
HF_INPLACE      = $01           ; Runs in place in the paged ROM
HF_BOOT         = $02           ; A driver the kernel starts at boot

; ---- The module directory: paged ROM bank 0, after the partition table's block (tools/romimg.js writes it)
MD_BASE         = $A200
MD_MAGIC        = MD_BASE + 0   ; "HYMD"
MD_VERSION      = MD_BASE + 4
MD_COUNT        = MD_BASE + 5   ; Entries
MD_INIT         = MD_BASE + 6   ; The entry of the module to start as init ($FF: none)
MD_ENTRIES      = MD_BASE + 8   ; 16 bytes each:
ME_BANK         = 0             ;   its first bank
ME_BANKS        = 1             ;   banks it spans
ME_TYPE         = 2             ;   HT_*
ME_FLAGS        = 3             ;   HF_*
ME_NAME         = 4             ;   (12) its name, zero-padded
ME_SIZE         = 16
MD_MAX          = 31
