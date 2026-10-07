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
PROG_RAM        = $0400         ; A task's RAM for its program: from here (below: its stack and OS area) ...
PROG_LOAD       = $0800         ;   and a RAM program's image from here at the lowest

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
TK_WOKEN        = $AF           ; A WAKE came while it wasn't waiting: its next PAUSE returns at once
F_SRV           = $B0           ; The file calls' scratch: the server of the request being made ...
F_CH            = $B1           ;   its channel ...
F_FD            = $B2           ;   its fd ...
F_LEFT          = $B3           ; (2)   the count still to move ...
F_TOTAL         = $B5           ; (2)   and the count moved
F_CHUNK         = $B7           ; (2) The count asked for in one request
TQ_WHO          = $B9           ; NOTE_POST's scratch (its caller's: a task's, or an irq entry's): the task or
TQ_MASK         = $BA           ;   group, the note's bit in its byte ...
TQ_INDEX        = $BB           ;   its byte in TK_NOTES ...
TQ_POSTED       = $BC           ;   and the tasks it went to
TK_EVENT        = $BD           ; Its event count (TASK_EVENT): a server adds 1 when something its clients may be
                                ;   waiting for has happened (a byte in, room to send), even in its irq entry
TK_EVTASK       = $BE           ; ST_EVENT: the server whose event count it waits on ...
TK_EVV          = $BF           ;   and the count it waits to see change
F_NMEM          = $C0           ; The file calls' name: its mount point's members (1: a # name) ...
F_EXACT         = $C1           ;   <> 0: the name is the mount point itself (a union directory, if it has more) ...
F_ROOT          = $C2           ;   and its root's length, as it's cleaned (1: /, 2: #x)
L_FD            = $C3           ; SPAWN's scratch (load.s), in the caller: the program's fd ...
L_FLAGS         = $C4           ;   its flags ...
L_ARGS          = $C5           ; (2)   the arguments ...
L_ARGLEN        = $C7           ;   their length with the 0 (0: none) ...
L_TASK          = $C8           ;   and the child.  The loader's, in the child: ...
L_AT            = $C9           ; (2)   where the next bytes go ...
L_LEFT          = $CB           ; (2)   and how many are still to come
L_PATH          = $CD           ; (2) SPAWN's, in the caller: the program's path (a name for one whose header has none)
TM_PTR          = $CF           ; (2) TASKMEM's, as kcopy's KC_PTR: this task's end of the copy ...
TM_PARTNER      = $D1           ;   the other task ...
TM_OLDBANK      = $D2           ;   and (the task read) its own RAM bank, while the copy has its bank register
KM_PTR          = $D3           ; (2) The kernel task's, a moment: where a byte of the kernel's messages goes (cons.s)
F_GONE          = $D5           ; The file calls' name: what its members said (1: one hadn't it; 2: one's device was gone)
TK_STEPBUF      = $D6           ; (16) The debugger's step (debug.s): its instruction, run out of line, and BRKs ...
TK_STEPNEXT     = $E6           ; (2)   where the instruction goes on to ...
TK_STEPTGT      = $E8           ; (2)   and where it goes taken (a branch), or an indirect JMP's target (its code's) ...
TK_STEPHIT      = $EA           ;   the BRK that means taken: its PC's low byte, as it pushes it.  0: no step out
OS_ZP_END       = $EB           ; (Checked: the OS zero page's end, at most $100)
.assert     OS_ZP_END <= $100, error, "The OS zero page runs past $FF"

; Task states (TK_STATE): the state of the context on top of the task's stack.  Only ST_READY runs (and
; ST_SLEEP once its time has come, ST_BLOCKED once its task is free, ST_EVENT once its server's event count has
; changed: the scheduler makes them ST_READY)
ST_FREE         = 0             ; Not in use
ST_READY        = 1             ; Runnable
ST_WAIT         = 2             ; Waiting for a wake (WAKE, a wait mask)
ST_CALL         = 3             ; Calling another task (SCALL): runs again when the call returns
ST_IDLE         = 4             ; A driver between requests: runs only for its calls and interrupts
ST_NEW          = 5             ; Being set up
ST_SLEEP        = 6             ; Sleeping till a tick count (K_WAKE_LO/HI, the kernel task's)
ST_BLOCKED      = 7             ; Waiting to SCALL a busy task (TK_BLOCKEDON)
ST_EVENT        = 8             ; Waiting for its server's event count to change (TK_EVTASK, TK_EVV), or a wake
TF_DRIVER       = $01           ; TK_FLAGS: a driver ...
TF_HOLD         = $10           ;   started by SPAWN_STOPPED: it stops at its entry point (task.s) ...
TF_BREAKS       = $20           ;   the debugger's breakpoints: any BRK stops it (TASKSTEP's TS_BREAKS) ...
TF_TRAP         = $40           ;   a BRK goes to K_TRAP (a step out, or TF_BREAKS: debug.s) ...
TF_STOPPED      = $80           ;   stopped (/proc's ctl, TASKSTOP): the scheduler passes it by till it's started
BUSY_NOSERVE    = $80           ; TK_BUSY: no serve entry (a program): a call to it is E_NODEV
IRQ_RESCHED     = $80           ; An irq entry's answer: a task switch, please (it woke a task)
LINE_VIA_T2     = 16            ; A line of its own for VIA timer 2 (the VIA's line is the tick's): the VIA's stub
                                ;   sends timer 2's interrupts here (common.s: IRQ_VIA).  The console's pacing
LINE_VIA_CA1    = 17            ; And one for the VIA's CA1 (an edge on J27's pin 11): the GPIO driver's, while it
                                ;   has /ca1 open (owning it turns CA1's interrupt on)
IRQ_LINES       = LINES + 2     ; The lines with owners: the hardware's, LINE_VIA_T2 and LINE_VIA_CA1

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
TA_PUTC         = $0227         ; PUTC's and GETC's byte (to and from fds 1 and 0)
TA_BRK          = $0228         ; (2) The end of its data area (BREAK) ...
TA_BRKMIN       = $022A         ; (2)   and the lowest it may go: its module's HX_TOP
TA_NOTIFY       = $022C         ; (2) Its note handler (NOTIFY), or 0
TA_NAME         = $0230         ; (16) Its name, zero-terminated
TA_OWNERS       = $0240         ; (18: IRQ_LINES) Each IRQ line's owner task ($FF: none): a copy in every task, so
                                ;   the IRQ dispatcher reads it where it is (IRQ_OWN writes them all)
TA_PAGEMAP      = $0252         ; (16) Its pages given out by PAGES_ALLOC (bit = page)
TA_BANKMAP      = $0262         ; (30) Its banks given out by BANKS_ALLOC (bit = bank, $00-$EF)
TA_SCRATCH      = $0280         ; (32) The call stubs' scratch: what a call copies in or out
TA_REQ          = $02A0         ; (32) A request it's making (RQ_*: the server takes it with SRV_TAKE)
TA_PATH         = $02C0         ; (64) The path a request names: the rest after its device (a server's copy too)
TA_FD           = $0300         ; (16) Its fds: a channel number each ($FF: closed)
TA_CWD          = $0310         ; (64) Its current directory (CHDIR), zero-terminated
TA_ARGS         = $0350         ; (176) Its arguments: zero-terminated strings, an empty one after the last (SPAWN
                                ;   copies them here; r0 at the start)
TA_ARGS_MAX     = 175
PATH_MAX        = 63            ; (A path: 63 characters and the 0)
FD_MAX          = 16

; ---- The kernel task's program zero page (task 0's $22-$7F)
K0_TICKS        = $22           ; (4) The tick count (TICK_HZ a second; TICKS gives its low 16 bits)
K0_SPAWNF       = $27           ; SPAWN's flags (SPAWN_*), in its KCALL
K0_BOOT         = $28           ; (4) The clock at tick 0: seconds since 2000-01-01 (time.s: TIME is it + the ticks
                                ;   since, in seconds)
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
K0_NQ_ANY       = $41           ; <> 0: notes to groups that irq entries queued (NOTE_QUEUE) for the kernel task to
K0_NQ           = $42           ; (16)  post (notes.s; the scheduler runs it while there are any): each group's
                                ;   (bit n - 1: note n, 1-8)
K0_SA           = $52           ; (2) Namespaces' KCALLs (ns.s): a string ...
K0_SB           = $54           ; (2)   another (a name) ...
K0_SC           = $56           ; (2)   a task's 64 bytes in K_RES or K_CAND ...
K0_NC           = $58           ;   the calling task ...
K0_NN           = $59           ;   its namespace ...
K0_NF           = $5A           ;   a mount point (a string) ...
K0_NL           = $5B           ;   a length ...
K0_NV           = $5C           ;   the flags, or the mode ...
K0_NW           = $5D           ;   BIND's source, UNMOUNT's what ...
K0_NS1          = $5E           ;   the source union's namespace and mount point ...
K0_NS2          = $5F
K0_NQS          = $60           ;   the next seq to give ...
K0_NT           = $61           ;   a count, a candidate's flags ...
K0_NR           = $62           ;   ns_rel's namespace ...
K0_C1           = $63           ;   the lowest seq at a mount point ...
K0_C2           = $64           ;   and the highest
K0_PN           = $65           ; pick's: the namespace ...
K0_PF           = $66           ;   the mount point ...
K0_PA           = $67           ;   the seq to pick after (0: the first) ...
K0_PB           = $68           ;   the lowest seq after it so far ...
K0_PE           = $69           ;   and its entry
K0_CO           = $6A           ; ns_clone's: the namespace copied ...
K0_CN           = $6B           ;   and the copy
K0_EP           = $6C           ; (2) The environments' KCALLs (env.s): a pointer (K0_PTR and K0_PTR2 theirs too) ...
K0_EC           = $6E           ;   the calling task ...
K0_ENL          = $6F           ;   and the name's length
K0_END          = $70
K0_POST         = $70           ; (16) POST's scratch, at boot only
K0_STEP         = $70           ; (8) TASKSTEP's (debug.s), in its KCALL: POST's bytes, after the boot
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
TR_ARGS         = 0             ; TASKREAD: a task's arguments (TA_ARGS_MAX + 1 bytes) ...
TR_CWD          = 1             ;   its current directory (PATH_MAX + 1 bytes) ...
TR_ENV          = 2             ;   its environment's first variables, whole, and a 0 (ENV_SIZE bytes at most) ...
TR_FRAME        = 3             ;   its state and the frame it left (TF_SIZE bytes) ...
TR_ENVAT        = 4             ;   or part of its environment (r1 bytes at most, from r2; .A/.X its bytes in use) ...
TR_FD           = 5             ;   or its fd r2 (FI_SIZE bytes: FI_*; FD2PATH's, TR_FD | TR_NAME, its name alone).
TR_NAME         = $80
FI_NAME         = 0             ;   TR_FD's record: the name its file was opened by (PATH_MAX + 1) ...
FI_MODE         = 64            ;   its open mode ...
FI_DEV          = 65            ;   its server's device letter ...
FI_QTYPE        = 66            ;   its qid type ...
FI_OFFSET       = 67            ;   and its offset (4)
FI_SIZE         = 71
                                ;   TR_FRAME's record:
TF_STATE        = 0             ;   its state (ST_*) ...
TF_S            = 1             ;   its stack pointer, as it was before the frame ...
TF_U            = 2             ;   the frame: U, Y, W, X, A, P, PC (2) ...
TF_Y            = 3
TF_W            = 4
TF_X            = 5
TF_A            = 6
TF_P            = 7
TF_PC           = 8
TF_RAM          = 10            ;   and its bank registers: RAM ($00) ...
TF_ROM          = 11            ;   and paged ROM ($01)
TF_SIZE         = 12
TM_READ         = 0             ; TASKMEM: from the task's memory ...
TM_WRITE        = $01           ;   or to it ...
TM_BANK         = $80           ;   (flag) in its RAM bank r3, at $8000-$9FFF
TS_STEP         = 0             ; TASKSTEP: one instruction ...
TS_NEXT         = 1             ;   one, a JSR's subroutine whole ...
TS_BREAKS       = 2             ;   from now, its BRKs stop it (the debugger's breakpoints) ...
TS_NOBREAKS     = 3             ;   or they're its own again (the note)
NI_FROM         = 0             ; NSINFO's answer: the mount point (PATH_MAX + 1) ...
NI_PATH         = 64            ;   the path in the device ...
NI_DEV          = 128           ;   the device letter ...
NI_SPEC         = 129           ;   the spec (8, zero-padded, and a 0) ...
NI_FLAGS        = 138           ;   the flags (MCREATE) ...
NI_SEQ          = 139           ;   and its place among the union's members
NI_SIZE         = 140

; ---- Files: the request block (TA_REQ in the client, TA_INBOX in the server: docs/reimplementation-from-
; scratch.md, appendix B), its requests, the open modes, the stat record
RQ_TYPE         = 0             ; The request (R_*)
RQ_FID          = 1             ; The fid (in); the new fid (out: R_OPEN, R_CREATE)
RQ_MODE         = 2             ; The open mode (O_*), or the channel's
RQ_CLIENT       = 3             ; The client task (the kernel fills it in)
RQ_U            = 4             ; The client's U
RQ_FLAGS        = 5             ; RF_*
RQ_OFFSET       = 6             ; (4)
RQ_COUNT        = 10            ; (2) The count asked for
RQ_DONE         = 12            ; (2) The count done (out)
RQ_BUF          = 14            ; (2) The buffer, in the client's view
RQ_PERM         = 16            ; R_CREATE: the new file's mode (DM_DIR for a directory); out: R_OPEN, R_CREATE:
                                ;   the file's qid type (QT_DIR ...)
RQ_NAMELEN      = 17            ; The name's length (R_OPEN, R_CREATE, R_REMOVE: the name is the client's TA_PATH)
RQ_GROUP        = 18            ; The client's note group
RQ_DEV          = 19            ; The device letter
RQ_SPEC         = 20            ; (8) The mount's spec, zero-padded
RQ_EVENT        = 28            ; The server's event count as it took the request (SRV_TAKE): after E_AGAIN, the
                                ;   client waits for it to change (ST_EVENT)
RQ_SIZE         = 32
R_OPEN          = 1             ; The requests
R_CREATE        = 2
R_READ          = 3
R_WRITE         = 4
R_CLUNK         = 5
R_STAT          = 6
R_WSTAT         = 7
R_REMOVE        = 8
R_FLUSH         = 9
R_DUP           = 10
RF_NONBLOCK     = $01           ; RQ_FLAGS: answer E_AGAIN rather than wait
O_READ          = 0             ; Open modes
O_WRITE         = 1
O_RDWR          = 2
O_RW_MASK       = 3
O_TRUNC         = $10
O_NONBLOCK      = $40
SR_NAME         = 0             ; The stat record (64 bytes): its name (32) ...
SR_QTYPE        = 32            ;   its qid: type (QT_*) ...
SR_QVERS        = 33            ;     version ...
SR_QPATH        = 34            ; (4)   and path (unique in its server) ...
SR_MODE         = 38            ; (2) its mode (permissions; DM_DIR) ...
SR_LENGTH       = 40            ; (4) its length ...
SR_MTIME        = 44            ; (4) its time (seconds since 2000) ...
SR_DEV          = 48            ;   its device letter ...
SR_INST         = 49            ;   and instance (the spec's first character)
SR_SIZE         = 64
QT_DIR          = $80           ; Qid types
QT_FILE         = $00
DM_DIR          = $80           ; The mode's high byte: a directory
IO_UNIT         = 512           ; The most a READ or WRITE asks of a server in one request

; Notes (numbers, as Plan 9's are strings): what they are, and the exit code of a task a note ends
NOTE_INTERRUPT  = 1             ; Ctrl-C (130)
NOTE_KILL       = 2             ; Kill: not caught (137)
NOTE_HANGUP     = 3             ; (129)
NOTE_ALARM      = 4             ; (142)
NOTE_BRK        = 5             ; sys: brk, a BRK instruction (133)
NOTE_USER       = 16            ; 16-31: the programs' own (128 + the note)
NOTE_MAX        = 31
NOTE_GROUP      = $80           ; NOTE's .A: a note group, not a task
SPAWN_NEWGROUP  = $01           ; SPAWN's flags: the child starts a note group of its own ...
SPAWN_NEWNS     = $02           ;   and a namespace of its own, empty (else it shares its parent's) ...
SPAWN_FDMAP     = $04           ;   and its fds r2's map (else the caller's 0, 1 and 2) ...
SPAWN_NOENV     = $08           ;   and an empty environment (else a copy of its parent's) ...
SPAWN_STOPPED   = $10           ;   and it stops at its entry point, for the debugger (TF_HOLD)
SPAWN_LOAD      = $80           ; (The kernel's own, in its KCALL: a RAM program, its file the child's LOAD_FD)
SPAWN_FDS       = 15            ; SPAWN's fd map: the child's fds it gives, at most ...
LOAD_FD         = 15            ;   and the child's fd that's its program's file as it starts (the loader's)
SP_NAME         = 0             ; SPAWN's staging, in the caller's TA_SCRATCH: the program's name (12, zero-padded) ...
SP_MAP          = 16            ;   and the fd map (FD_MAX: the caller's fd for each of the child's, $FF: closed)
CH_MAX          = 48            ; The channels: open files, shared by the fds that name them ...
K_CH_REFS       = $0800         ; (48)   the fds naming it (0: free) ...
K_CH_SRV        = $0830         ; (48)   its server's task ($FF: the server ended) ...
K_CH_DEV        = $0860         ; (48)   its device letter ...
K_CH_FID        = $0890         ; (48)   the server's fid ...
K_CH_MODE       = $08C0         ; (48)   its open mode ...
K_CH_QTYPE      = $08F0         ; (48)   its qid type (QT_DIR ...) ...
K_CH_OFF0       = $0920         ; (48)   and its offset (4 bytes, low first)
K_CH_OFF1       = $0950         ; (48)
K_CH_OFF2       = $0980         ; (48)
K_CH_OFF3       = $09B0         ; (48)
DEV_MAX         = 32            ; The servers' device letters (SRV_REGISTER) ...
K_DEV_LETTER    = $0A00         ; (32)   the letter (0: a free entry) ...
K_DEV_TASK      = $0A20         ; (32)   and its server's task
K_SEG_FIRST     = $0740         ; (16) Each shared segment's first shared bank ID ...
K_SEG_COUNT     = $0750         ; (16)   its banks (0: the segment isn't in use) ...
K_SEG_REFS      = $0760         ; (16)   and the tasks attached
K_SEGATT_LO     = $0770         ; (16) Each task's segments attached (bit = segment): 0-7 ...
K_SEGATT_HI     = $0780         ; (16)   8-15
K_SHMAP         = $0790         ; (16) The shared banks given to segments (bit = ID - SEG_IDS)
SEG_MAX         = 16
SEG_IDS         = $80           ; Segments' shared bank IDs: $80-$FF (U = ID >> 4, $00 = $F0 | ID & $0F); $00-$7F
                                ;   are the shared RAM disk's (phase 4)
SEM_MAX         = 16            ; The semaphores (sem.s): each one's maker ($FF: free) ...
K_SEM_MAKER     = $0B50         ; (16)
K_SEM_COUNT     = $0B60         ; (16)   its count ...
K_SEM_FLAGS     = $0B70         ; (16)   SEM_MUTEX ...
K_SEM_HOLDER    = $0B80         ; (16)   a mutex's holder ($FF: none) ...
K_SEM_WAITLO    = $0B90         ; (16)   and the tasks waiting for it (bit = task): 0-7 ...
K_SEM_WAITHI    = $0BA0         ; (16)   8-15
SEM_MUTEX       = $01           ; SEM_NEW's flag: a mutex
K_HWT_WORD      = $0BB0         ; (4) REBOOT_HWTEST's word, "HWT!", for POST after the restart (task RAM: it stays)
REBOOT_HWTEST   = $01           ; REBOOT's flag: into the hardware test
K_EXIT_MSG      = $0500         ; (16 x 32) The exit records' messages
EXIT_MSG_MAX    = 31            ;   (31 characters and the 0)
EX_NONE         = 0
EX_HELD         = 1

; ---- Namespaces (kernel/ns.s, BIOS ROM page 3).  A namespace is the mount entries with its number; a task names
; one (tasks share one till one of them changes it: then it gets a copy).  An entry is one member of the union at
; a mount point: a server's device letter and spec, and a path in that server.  The mount points and the paths are
; strings in a pool, shared, each counted by the entries using it
NS_MAX          = 16            ; Namespaces (one a task: each shell has its own) ...
K_NS_REFS       = $3200         ; (16)   the tasks (and union channels) using it (0: free)
K_TASK_NS       = $0A48         ; (16) Each task's namespace ($FF: none: only # names)
K_RES_FROM      = $0A58         ; (16) Each task's name being resolved (in K_RES): its mount point (a string) ...
K_RES_LEN       = $0A68         ; (16)   the length of the part that matched it ...
K_RES_STATE     = $0A78         ; (16)   RS_* ...
K_RES_SEQ       = $0A88         ; (16)   the last member tried (its K_MT_SEQ) ...
K_RES_NS        = $0A98         ; (16)   and the namespace
K_CH_UFROM      = $0AC0         ; (48) A channel on a union directory: the mount point ($FF: not a union) ...
K_CH_UNS        = $0AF0         ; (48)   its namespace ...
K_CH_USEQ       = $0B20         ; (48)   and the member it's reading
K_CH_NAME       = $4200         ; (48 x 64) Each channel's name, as it was opened (FD2PATH, /proc/N/fd)
.assert     <K_CH_NAME = 0 .and K_CH_NAME + 64 * CH_MAX <= K_KMESG_BUF, error, "K_CH_NAME: page-aligned, below the kernel's messages"
MT_MAX          = 255           ; The mount entries (a shell's namespace has about 25; an entry is a byte, $FF none) ...
K_MT_NS         = $3300         ; (255)  the namespace ($FF: a free entry) ...
K_MT_FROM       = $3400         ; (255)  the mount point (a string) ...
K_MT_SEQ        = $3500         ; (255)  its order among the union's members (lowest first) ...
K_MT_DEV        = $3600         ; (255)  the device letter ...
K_MT_PATH       = $3700         ; (255)  the path in the device (a string, with no leading /) ...
K_MT_FLAGS      = $3800         ; (255)  MCREATE ...
K_MT_SPEC       = $3900         ; (8 x 255) and the spec, zero-padded: byte n of entry e at K_MT_SPEC + MT_MAX * n + e
.assert     K_MT_SPEC + 8 * MT_MAX <= $4100, error, "The mount entries run past their room ($3300-$40FF)"
SP_MAX          = 64            ; The strings (PATH_MAX characters and the 0, 64 bytes each) ...
K_SP_REFS       = $1280         ; (64)   the entries using it (0: free) ...
K_SP            = $1400         ; (64 x 64) the strings
K_RES           = $2400         ; (16 x 64) Each task's name being resolved: absolute and clean, or a # name
K_CAND          = $2800         ; (16 x 64) Each task's last candidate (BIND's new): its path in its server ...
K_CAND_DS       = $2C00         ; (16 x 16)  its device letter and spec (9 bytes: as RQ_DEV and RQ_SPEC are)
K_NS_PATH       = $2D00         ; (64) A path, a moment (BIND's old)
K_ENVNAME       = $2D40         ; (32) An environment variable's name (env.s) ...
K_ENV_EB        = $2D60         ; (2)   the environment it's in ...
K_ENV_EL        = $2D62         ; (2)   its value's length ...
K_ENV_OFF       = $2D64         ; (2)   an offset in it ...
K_ENV_N         = $2D66         ; (2)   a count ...
K_ENV_U         = $2D68         ; (2)   the environment's bytes in use (with its end) ...
K_ENV_MN        = $2D6A         ; (2)   and a move's count (e_move)
K_ENV_SRC       = $2D6C         ; (2) The caller's bytes (ENV_PUT) or buffer (ENV_GET)
K_XBUF          = $2E00         ; (176) TASKREAD's bytes, or NSINFO's answer, on their way to the caller
; Each task's environment is a RAM bank of the kernel task's own, at $8000 as env.s works on it (task t's: bank
; K_ENV_BASE + t, the first good RAM module's 16): its variables, each its name's length (1-ENV_NAME_MAX), its name,
; its value's length (2) and its value; ending with 0.  ENV_MAX bytes at most (its 0 too); TASKREAD's TR_ENV gives its
; first variables, whole, in ENV_SIZE
K_ENV_BASE      = $3000         ; The environments' first bank ($m0: module m's), or $FF: no RAM module (none)
K_ENV_ZERO      = $3001         ; A 0 (TASKREAD's TR_ENV puts it after the variables that fit)
K_KMESG_HEAD    = $3002         ; (2) The kernel's messages (KMESG, /dev/kmesg): where the next byte goes in the ring ...
K_KMESG_LEN     = $3004         ; (2)   and the bytes held (KMESG_SIZE at most: then the oldest go)
K_KMESG_BUF     = $7000         ; (KMESG_SIZE) The ring: what the kernel prints on the bring-up console, the boot's too
KMESG_SIZE      = $1000
.assert     <K_KMESG_BUF = 0 .and (>K_KMESG_BUF & >(KMESG_SIZE - 1)) = 0 .and K_KMESG_BUF + KMESG_SIZE <= $8000, error, "K_KMESG_BUF: a ring on a KMESG_SIZE boundary"
K_ENV_BOUNCE    = $3100         ; (256) A copy from one environment's bank to another's (SPAWN's), on its way
ENV_WINDOW      = BANK_WINDOW   ; (Where an environment is, its bank selected)
ENV_MAX         = BANK_SIZE
ENV_SIZE        = 1024
ENV_NAME_MAX    = 31
ENV_NONE        = $FF
RS_NONE         = 0             ; K_RES_STATE: nothing found
RS_NS           = 1             ;   a mount point found: its members, in order
RS_DEV          = 2             ;   a # name: one candidate, its device
RS_DONE         = 3             ;   no more candidates
MREPL           = 0             ; BIND's and MOUNT's flags: in place of what's there ...
MBEFORE         = 1             ;   a union, this first ...
MAFTER          = 2             ;   a union, this last ...
MORDER          = 3
MCREATE         = 4             ;   and a CREATE in the union comes here

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
MD_MAX          = 127           ; ($A200-$A9FF: 31 till phase 4.3, when the tools needed more)
