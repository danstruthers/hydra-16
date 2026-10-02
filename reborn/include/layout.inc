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
KF_PAGE         = $8C           ; The kernel's far call (common.s: K_FAR): the routine's BIOS ROM page ...
KF_VEC          = $8D           ; (2)   its address ...
KF_A            = $8F           ;   and .A, a moment
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
TK_NOTED        = $A2           ; <> 0: a note is pending (TK_NOTES)
TK_NOTES        = $A3           ; (4) The notes pending (bit n = note n, 1-31)
TK_INNOTE       = $A7           ; <> 0: its note handler is running (more notes wait for it)
TN_A            = $A8           ; A system call's results, a moment, on its way out through the notes
TN_X            = $A9
TN_Y            = $AA
TN_P            = $AB
K_FNVEC         = $AC           ; (2) KCALL: the routine the caller names (the kernel task reads it here) ...
K_FNPAGE        = $AE           ;   and its BIOS ROM page
OS_ZP_END       = $AF           ; (Checked: the OS zero page's end, at most $100)
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
TA_BRK          = $0228         ; (2) The end of its data area (BREAK) ...
TA_BRKMIN       = $022A         ; (2)   and the lowest it may go: its module's HX_TOP
TA_NOTIFY       = $022C         ; (2) Its note handler (NOTIFY), or 0
TA_NAME         = $0230         ; (16) Its name, zero-terminated
TA_OWNERS       = $0240         ; (16) Each IRQ line's owner task ($FF: none): a copy in every task, so the IRQ
                                ;   dispatcher reads it where it is (IRQ_OWN writes them all)
TA_PAGEMAP      = $0250         ; (16) Its pages given out by PAGES_ALLOC (bit = page)
TA_BANKMAP      = $0260         ; (30) Its banks given out by BANKS_ALLOC (bit = bank, $00-$EF)
TA_ARGS         = $0300         ; (256) Its arguments, zero-terminated (SPAWN copies them here; r0 at the start)
TA_ARGS_MAX     = 255

; ---- The kernel task's program zero page (task 0's $22-$7F)
K0_TICKS        = $22           ; (4) The tick count (TICK_HZ a second; TICKS gives its low 16 bits)
K0_SPAWNF       = $27           ; SPAWN's flags (SPAWN_*), in its KCALL
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
K0_BADMODS      = $3D           ; (2) POST: the RAM modules with bad lines (bit = module): not used
K0_BADSHARED    = $3F           ; POST: the shared RAM chips with bad lines (bit = chip, 0-3): not used
K0_POSTFAULT    = $40           ; POST: what it found wrong (PF_*), 0 for nothing
K0_END          = $41
K0_POST         = $70           ; (16) POST's scratch, at boot only
PF_T            = $01           ; K0_POSTFAULT: a T line (tasks' memory not apart)
PF_SHARED       = $02           ;   shared RAM not shared between tasks
PF_W            = $04           ;   a W line (BIOS ROM pages)
PF_RAM          = $08           ;   a RAM module or shared chip with bad lines (left unused)
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
K_NGROUP        = $0490         ; (16) Each task's note group (a task's number: the group's first)
K_CPU_LO        = $04A0         ; (16) Each task's CPU time, in ticks (24 bits: the tick charges the task it
K_CPU_MID       = $04B0         ; (16)   interrupts)
K_CPU_HI        = $04C0         ; (16)

; TASKINFO's answer (24 bytes)
TI_STATE        = 0             ; ST_*
TI_FLAGS        = 1             ; TF_*
TI_PARENT       = 2             ; Its parent ($FF: none)
TI_CPU          = 3             ; (3) Its CPU time, in ticks
TI_BANK         = 6             ; Its module's paged ROM bank ($FF: none)
TI_TYPE         = 7             ; Its module's type (HT_*), 0: none
TI_NAME         = 8             ; (16) Its name, zero-terminated
TI_GROUP        = 24            ; Its note group
TI_SIZE         = 25

; Notes (numbers, as Plan 9's are strings): what they are, and the exit code of a task a note ends
NOTE_INTERRUPT  = 1             ; Ctrl-C (130)
NOTE_KILL       = 2             ; Kill: not caught (137)
NOTE_HANGUP     = 3             ; (129)
NOTE_ALARM      = 4             ; (142)
NOTE_BRK        = 5             ; sys: brk, a BRK instruction (133)
NOTE_USER       = 16            ; 16-31: the programs' own (128 + the note)
NOTE_MAX        = 31
NOTE_GROUP      = $80           ; NOTE's .A: a note group, not a task
SPAWN_NEWGROUP  = $01           ; SPAWN's flags: the child starts a note group of its own
K_SEG_FIRST     = $0740         ; (16) Each shared segment's first shared bank ID ...
K_SEG_COUNT     = $0750         ; (16)   its banks (0: the segment isn't in use) ...
K_SEG_REFS      = $0760         ; (16)   and the tasks attached
K_SEGATT_LO     = $0770         ; (16) Each task's segments attached (bit = segment): 0-7 ...
K_SEGATT_HI     = $0780         ; (16)   8-15
K_SHMAP         = $0790         ; (16) The shared banks given to segments (bit = ID - SEG_IDS)
SEG_MAX         = 16
SEG_IDS         = $80           ; Segments' shared bank IDs: $80-$FF (U = ID >> 4, $00 = $F0 | ID & $0F); $00-$7F
                                ;   are the shared RAM disk's (phase 4)
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

; ---- The hardware test: a standalone program in paged ROM bank 1, entered at $A000 (os_rom/hwtest, unchanged:
; tools/romimg.js copies it, and fills its table of the ROMs' checksums at $DF00)
HWT_BANK        = $01
HWT_ENTRY       = $A000

; ---- The module directory: paged ROM bank 0, after the partition table's block (tools/romimg.js writes it)
MD_BASE         = $A200
MD_MAGIC        = MD_BASE + 0   ; "HYMD"
MD_VERSION      = MD_BASE + 4
MD_COUNT        = MD_BASE + 5   ; Entries
MD_INIT         = MD_BASE + 6   ; The entry of the module to start as init ($FF: none)
MD_HWTEST       = MD_BASE + 7   ; <> 0: bank HWT_BANK holds the hardware test (a T typed during POST starts it)
MD_ENTRIES      = MD_BASE + 8   ; 16 bytes each:
ME_BANK         = 0             ;   its first bank
ME_BANKS        = 1             ;   banks it spans
ME_TYPE         = 2             ;   HT_*
ME_FLAGS        = 3             ;   HF_*
ME_NAME         = 4             ;   (12) its name, zero-padded
ME_SIZE         = 16
MD_MAX          = 31
