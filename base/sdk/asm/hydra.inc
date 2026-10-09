; hydra.inc - the Hydra-16's system calls, error codes and constants, for programs in assembly.  Made by tools/apigen.js from spec/: don't edit
; The rules (reborn/docs/conventions.md): call with jsr; .A, .X, .Y and r0-r15 in and out; 16-bit results
; in .A (low) and .X (high); C = 0 success, C = 1 failure with the error code in .A.  Calls change .A, .X, .Y,
; r0-r15 and the flags; never $22-$7F (the program's zero page).

; ---- system: System information and services
SYSINFO         = $F800       ; -
                              ; -> .A = the ABI version (1); .X = how many RAM modules are installed; r0 = the free tasks (bit = task)
ERRSTR          = $F803       ; .A = an error code; r0 = a buffer (32 bytes)
                              ; -> the error's text in the buffer, zero-terminated ("error $xx" for an unknown code)
KMESG           = $F806       ; r0 = a buffer; r1 = its size, the most it copies; r2 = an offset in the messages held, 0 the oldest
                              ; -> .A/.X = the bytes held, KMESG_SIZE at most; the buffer has those from r2, r1 at most
REBOOT          = $F809       ; .A = 0, or REBOOT_HWTEST
                              ; -> (doesn't return)
XCALL           = $F80C       ; r15 = a routine's address ($A000-$DFFF); r14 (its low byte) = the paged ROM bank it's in; .A, .X, .Y, the flags and r0-r13: the routine's
                              ; -> .A, .X, .Y, the flags and r0-r15 as the routine leaves them

; ---- task: Tasks: starting and ending them, waiting, scheduling, ticks
EXITS           = $F830       ; .A = the exit code (0: success); r0 = a message (zero-terminated, 31 characters at most), or 0 for none
                              ; -> (doesn't return)
WAIT            = $F833       ; .A = a task this one started, or $FF for any; r0 = a buffer for the message (32 bytes), or 0
                              ; -> .A = the task that ended; .X = its exit code; its message in r0's buffer ("" for none)
SPAWN           = $F836       ; r0 = the program's path (through the namespace: /bin/NAME; or #m/NAME, a module in the paged ROM); r1 = its arguments (zero-terminated strings, an empty one after the last: 176 bytes at most), or 0; .A = flags (SPAWN_NEWGROUP, SPAWN_NEWNS, SPAWN_FDMAP, SPAWN_NOENV, SPAWN_STOPPED); r2 = an fd map (with SPAWN_FDMAP)
                              ; -> .A = the new task
GETPID          = $F839       ; -
                              ; -> .A = this task's number
YIELD           = $F83C       ; -
SLEEP           = $F83F       ; .A/.X = ticks (TICK_HZ a second; 0-32767)
SLEEP_UNTIL     = $F842       ; .A/.X = a tick count (at most 32767 ticks ahead)
TICKS           = $F845       ; -
                              ; -> .A/.X = the tick count (TICK_HZ a second; it wraps after about 5.5 minutes)
PREEMPT_OFF     = $F848       ; -
PREEMPT_ON      = $F84B       ; -
TASKINFO        = $F84E       ; .A = a task (0-15); r0 = a buffer (TI_SIZE bytes)
                              ; -> the buffer: TI_STATE, TI_FLAGS, TI_PARENT, TI_CPU (3 bytes: ticks), TI_BANK, TI_TYPE, TI_NAME (16)
NOTIFY          = $F851       ; r0 = the note handler, or 0 for none
NOTE            = $F854       ; .A = a task, or NOTE_GROUP | a note group; .X = the note (1-31: NOTE_*)
MODINFO         = $F857       ; .A = an entry of the module directory (0 on); r0 = a buffer (ME_SIZE bytes)
                              ; -> the buffer: ME_BANK, ME_BANKS, ME_TYPE (HT_*), ME_FLAGS (HF_*), ME_NAME (12, zero-padded)
ENV_GET         = $F85A       ; .A = a task ($FF: this one); r0 = a variable's name; r1 = a buffer; r2 = its size; r3 = an offset in the value
                              ; -> .A/.X = the value's length from the offset (0 at its end or past it); as much of it as fits, in the buffer
ENV_PUT         = $F85D       ; .A = a task ($FF: this one); r0 = a variable's name; r1 = bytes; r2 = their count; r3 = where in its value they go (the value cut there first: 0 for a new value; its length at most)
ENV_DEL         = $F860       ; .A = a task ($FF: this one); r0 = a variable's name
ENV_NAME        = $F863       ; .A = a task ($FF: this one); .X = which variable (0 on); r0 = a buffer (ENV_NAME_MAX + 1 bytes)
                              ; -> its name in the buffer, zero-terminated; .A/.X = its value's length
TASKREAD        = $F866       ; .A = a task ($FF: this one); .X = what: TR_ARGS (its arguments: ARGS_MAX bytes), TR_CWD (its current directory: PATH_MAX + 1 bytes), TR_ENV (its environment's first variables, whole, and a 0: ENV_SIZE bytes at most), TR_FRAME (its state and the frame it left as it was switched out: TF_SIZE bytes, TF_*) or TR_ENVAT (part of its environment), or TR_FD (its fd r2: FI_SIZE bytes, FI_*); r0 = a buffer that big; r1 = TR_ENVAT's count (the most it copies); r2 = TR_ENVAT's offset, or TR_FD's fd
                              ; -> the bytes in the buffer; .A/.X = TR_ENVAT's: the environment's bytes in use (its 0 too: it copied as many as there are from r2, r1 at most)
GETPPID         = $F869       ; -
                              ; -> .A = this task's parent ($FF: none)
SEM_NEW         = $F86C       ; .A = its count: how many can take it before a taker waits (0-255); .X = 0, or SEM_MUTEX: a mutex (its count 1, and it has a holder: only the task that took it gives it back)
                              ; -> .A = the semaphore (0 to SEM_MAX - 1)
SEM_ACQUIRE     = $F86F       ; .A = a semaphore
SEM_TRY         = $F872       ; .A = a semaphore
SEM_RELEASE     = $F875       ; .A = a semaphore
SEM_FREE        = $F878       ; .A = a semaphore

; ---- memory: Memory: the break, pages, banks, shared segments (phase 1.8)
BREAK           = $F890       ; r0 = the new end of the program's data (the first byte past it), or 0 to ask
                              ; -> r0 = the end
PAGES_ALLOC     = $F893       ; .A = pages (256 bytes each)
                              ; -> r0 = the first one's address; .A = its page
PAGES_FREE      = $F896       ; .A = pages; r0 = the first one's address
BANKS           = $F899       ; -
                              ; -> .A = the banks the task has (16 for each good RAM module); r0 = the good modules (bit = module)
BANKS_ALLOC     = $F89C       ; .A = banks
                              ; -> .A = the first (for $00)
BANKS_FREE      = $F89F       ; .A = the first bank; .X = how many
SEG_CREATE      = $F8A2       ; .A = banks (1-128, 8K each)
                              ; -> .A = the segment (0-15)
SEG_ATTACH      = $F8A5       ; .A = a segment
SEG_DETACH      = $F8A8       ; .A = a segment
SEG_MAP         = $F8AB       ; .A = a segment; .X = one of its banks (0 on)
                              ; -> .A = the value for U; .X = the value for $00
SEGINFO         = $F8AE       ; -
                              ; -> .A = the shared banks segments can have (8K each: the good chips'); .X = those segments have now; r0 = the segments in use (bit = segment)
BANKS_ALLOC_IN  = $F8B1       ; .A = how many; .X = the lowest bank it may take; .Y = the highest (module m's are $m0-$mF)
                              ; -> .A = the first
SEG_CREATE_IN   = $F8B4       ; .A = how many banks (1-128); .X = the lowest shared bank ID it may take; .Y = the highest (U << 4 | the bank's low nibble; segments have $80-$FF)
                              ; -> .A = the segment

; ---- file: Files (phase 2)
OPEN            = $F8C0       ; r0 = a path; .A = the mode (O_READ, O_WRITE, O_RDWR; | O_TRUNC, O_NONBLOCK)
                              ; -> .A = an fd
CREATE          = $F8C3       ; r0 = a path; .A = the mode; .X = the new file's mode, high byte (DM_DIR: a directory)
                              ; -> .A = an fd
CLOSE           = $F8C6       ; .A = an fd
READ            = $F8C9       ; .A = an fd; r0 = a buffer; r1 = a count
                              ; -> .A/.X = the count read (0: the end of the file)
WRITE           = $F8CC       ; .A = an fd; r0 = the data; r1 = its count
                              ; -> .A/.X = the count written
SEEK            = $F8CF       ; .A = an fd; r0, r1 = an offset (low word, high word: signed); .X = from: 0 the start, 1 the offset, 2 the end
                              ; -> r0, r1 = the new offset
STAT            = $F8D2       ; r0 = a path; r1 = a buffer (SR_SIZE bytes)
                              ; -> the file's stat record in the buffer (SR_*)
FSTAT           = $F8D5       ; .A = an fd; r0 = a buffer (SR_SIZE bytes)
                              ; -> the stat record in the buffer
WSTAT           = $F8D8       ; r0 = a path; r1 = a stat record
FWSTAT          = $F8DB       ; .A = an fd; r0 = a stat record
REMOVE          = $F8DE       ; r0 = a path
DUP             = $F8E1       ; .A = an fd
                              ; -> .A = a new fd (the lowest free), on the same channel (the same offset)
DUP2            = $F8E4       ; .A = an fd; .X = another (0-15)
CHDIR           = $F8E7       ; r0 = a directory's path
GETCWD          = $F8EA       ; r0 = a buffer (PATH_MAX + 1 bytes)
                              ; -> .A = the current directory's length; the directory in the buffer, zero-terminated
PIPE            = $F8ED       ; -
                              ; -> .A = an fd to read from; .X = an fd to write to
FD2PATH         = $F8F0       ; .A = an fd; r0 = a buffer (PATH_MAX + 1 bytes)
                              ; -> the name in the buffer, zero-terminated

; ---- name: Namespaces (phase 2)
BIND            = $F920       ; r0 = new (a path); r1 = old (a path: absolute); .A = MREPL, MBEFORE or MAFTER, and MCREATE
MOUNT           = $F923       ; .X = a device letter; r0 = its spec (a string, 8 characters at most), or 0; r1 = old; .A = BIND's flags
UNMOUNT         = $F926       ; r0 = new, or 0; r1 = old
NSINFO          = $F929       ; .A = a task ($FF: this one); .X = which entry of its namespace (0 on); r0 = a buffer (NI_SIZE bytes)
                              ; -> the entry in the buffer (NI_*): its mount point, its device letter and spec, the path in that device, its flags (MCREATE) and its place among the union's members

; ---- cons: Console helpers: a byte or a string to stdout, a byte from stdin
PUTC            = $F950       ; .A = a byte
PUTS            = $F953       ; r0 = a string, zero-terminated
GETC            = $F956       ; -
                              ; -> .A = a byte
PUTHEX          = $F959       ; .A = a byte

; ---- time: The clock (phase 5)
TIME            = $F980       ; -
                              ; -> r0/r1 = the clock: seconds since 2000-01-01 00:00:00
TIME_SET        = $F983       ; r0/r1 = seconds since 2000-01-01 00:00:00
RTC             = $F986       ; .A = 0, read, or 1, write; r0 = 8 bytes: the DS1747's registers (RTC_REGS, hw.inc), BCD

; ---- server: Servers and drivers
IRQ_OWN         = $F9B0       ; .A = an IRQ line (0-14), or LINE_VIA_T2 (VIA timer 2), or LINE_VIA_CA1 (VIA CA1)
IRQ_RELEASE     = $F9B3       ; .A = an IRQ line
WAKE            = $F9B6       ; .A = a task
PAUSE           = $F9B9       ; -
NOTE_POST       = $F9BC       ; .A = a task, or NOTE_GROUP | a note group; .X = the note
SRV_REGISTER    = $F9BF       ; .A = a device letter (printable; not #)
SRV_TAKE        = $F9C2       ; -
                              ; -> .A = the request (R_*); .Y = the client; the request block in TASK_INBOX, and (R_OPEN, R_CREATE, R_REMOVE) the name in TASK_PATH
SRV_REPLY       = $F9C5       ; TASK_INBOX's RQ_FID, RQ_DONE, RQ_PERM, RQ_EVENT: what the request returns; .A, C: the answer
                              ; -> .A and C kept
CLIENT_READ     = $F9C8       ; r0 = here; r1 = there, in the client's view; r2 = a count
CLIENT_WRITE    = $F9CB       ; r0 = here (the data); r1 = there, in the client's view; r2 = a count
NOTE_QUEUE      = $F9CE       ; IRQs off; .X = a note group (0-15); .A = the note's bit: 1 << (note - 1), for a note 1-8
                              ; -> .X kept
ROMREAD         = $F9D1       ; .A = a paged ROM bank; r0 = where in it ($A000-$DFFF); r1 = a buffer (not at $A000-$DFFF); r2 = a count
                              ; -> the bytes in the buffer
TASKMEM         = $F9D4       ; .A = a task; .X = TM_READ or TM_WRITE, | TM_BANK; r0 = a buffer; r1 = an address in the task's view, $0000-$DFFF (with TM_BANK: $8000-$9FFF, in its RAM bank r3); r2 = a count (the range ends at $E000, or with TM_BANK at $A000, at most)
TASKSTOP        = $F9D7       ; .A = a task; .X = 1 to stop it, 0 to start it again
TASKSTEP        = $F9DA       ; .A = a task; .X = TS_STEP, TS_NEXT, TS_BREAKS or TS_NOBREAKS

; ---- dbg: Debugging and tests: unstable, may change in any version
DBG_SCALL       = $FA10       ; .Y = a driver task; .A, .X = what its serve entry gets
                              ; -> .A, .X and C from its serve entry
DBG_KCOPY       = $FA13       ; .A = another task; r0 = an address in this task; r1 = an address in that task; r2 = a byte count; C = 0 to copy from this task to that one, C = 1 the other way
DBG_PS          = $FA16       ; -

; ---- error codes
E_PERM          = $01         ; not allowed
E_INVAL         = $02         ; invalid argument
E_NOSYS         = $03         ; no such call
E_AGAIN         = $04         ; not yet: try again
E_INTR          = $05         ; interrupted
E_NOMEM         = $06         ; out of memory
E_BUSY          = $07         ; busy
E_RANGE         = $08         ; out of range
E_FAULT         = $09         ; bad address
E_NAMETOOLONG   = $0A         ; name too long
E_TOOBIG        = $0B         ; too big
E_NOENT         = $20         ; not found
E_EXIST         = $21         ; already exists
E_NOTDIR        = $22         ; not a directory
E_ISDIR         = $23         ; is a directory
E_NOTEMPTY      = $24         ; directory not empty
E_BADF          = $25         ; bad file descriptor
E_MFILE         = $26         ; no free file descriptor
E_NFILE         = $27         ; too many open files
E_NOSPC         = $28         ; disk full
E_ROFS          = $29         ; read-only
E_IO            = $2A         ; i/o error
E_NODEV         = $2B         ; no such device
E_PIPE          = $2C         ; broken pipe
E_NOEXEC        = $2D         ; not a program
E_EOF           = $2E         ; end of file
E_NOTFS         = $2F         ; no file system
E_MEDIA         = $30         ; medium error
E_NOTASK        = $40         ; no free task
E_SRCH          = $41         ; no such task
E_CHILD         = $42         ; no such child
E_NSFULL        = $50         ; namespace full
E_NSLOOP        = $51         ; too many binds

; ---- constants
ABI_VERSION     = 1           ; The ABI version this include describes
TICK_HZ         = 200         ; Ticks a second
INIT_TASK       = 1           ; init's task (and its note group: the console's foreground at first)
PROG_ZP         = $22         ; A program's own zero page: $22-$7F
PROG_ZP_END     = $80         
TASK_ARGS       = $0350       ; A task's arguments: zero-terminated strings, an empty one after the last (176 bytes at most)
ARGS_MAX        = 176         ; SPAWN's arguments: their bytes at most (each string's 0, and the empty one that ends them)
TASK_NAME       = $0230       ; A task's name, zero-terminated
IRQ_RESCHED     = $80         ; An irq entry's answer: a task switch, please
LINE_VIA_T2     = 16          ; IRQ_OWN's line for VIA timer 2 (the hardware's lines: hw.inc)
LINE_VIA_CA1    = 17          ; IRQ_OWN's line for VIA CA1 (J27's pin 11): owning it turns CA1's interrupt on; the owner clears its flag
TASK_EVENT      = $BD         ; A task's event count: a server adds 1 (inc TASK_EVENT) when what its clients wait for has happened
HT_PROGRAM      = 1           ; HYX2 module types
HT_DRIVER       = 2           
HT_LIBRARY      = 3           
HF_INPLACE      = $01         ; HYX2 flags: runs in place in the paged ROM
HF_BOOT         = $02         ; HYX2 flags: a driver the kernel starts at boot
HX_MAGIC        = 0           ; The HYX2 header (HX_SIZE bytes, a program's first): HYX2
HX_HSIZE        = 4           ; HYX2: the header's size (48)
HX_TYPE         = 5           ; HYX2: the type (HT_*)
HX_FLAGS        = 6           ; HYX2: the flags (HF_*)
HX_ABI          = 7           ; HYX2: the ABI version it was built for
HX_LOAD         = 8           ; HYX2: its load address (2: $A000 in place, $0800 a RAM program)
HX_LENGTH       = 10          ; HYX2: its image's length (2: in its last bank, in place)
HX_DATA_LOAD    = 12          ; HYX2: its data, where it is in the image (2) ...
HX_DATA_RUN     = 14          ; HYX2: ... where it runs (2) ...
HX_DATA_LEN     = 16          ; HYX2: ... and its length (2)
HX_BSS          = 18          ; HYX2: its BSS (2) ...
HX_BSS_LEN      = 20          ; HYX2: ... and its length (2: cleared as it starts)
HX_TOP          = 22          ; HYX2: the top of the RAM it uses (2: its break starts there)
HX_MAIN         = 24          ; HYX2: entry: main (a program), init (a driver)
HX_SERVE        = 26          ; HYX2: entry: serve (a driver)
HX_IRQ          = 28          ; HYX2: entry: irq (a driver)
HX_STOP         = 30          ; HYX2: entry: stop (a driver)
HX_DEVICE       = 32          ; HYX2: its device letter (a driver)
HX_BANKS        = 33          ; HYX2: the banks it spans (in place)
HX_VERSION      = 34          ; HYX2: its version (2)
HX_NAME         = 36          ; HYX2: its name (12: zero-terminated)
HX_SIZE         = 48          
ME_BANK         = 0           ; MODINFO's answer (ME_SIZE bytes): the module's first paged ROM bank
ME_BANKS        = 1           ; MODINFO: the banks it spans
ME_TYPE         = 2           ; MODINFO: its type (HT_*)
ME_FLAGS        = 3           ; MODINFO: its flags (HF_*)
ME_NAME         = 4           ; MODINFO: its name (12 bytes, zero-padded)
ME_SIZE         = 16          
TI_STATE        = 0           ; TASKINFO's answer: the task's state (0: free, 1 ready, 2 wait, 3 call, 4 idle, 5 new, 6 sleep, 7 blocked, 8 event)
TI_FLAGS        = 1           ; TASKINFO: its flags (TF_DRIVER, TF_STOPPED)
TF_DRIVER       = $01         ; TI_FLAGS: a driver
TF_STOPPED      = $80         ; TI_FLAGS: stopped (/proc's ctl: TASKSTOP)
TI_PARENT       = 2           ; TASKINFO: its parent ($FF: none)
TI_CPU          = 3           ; TASKINFO: its CPU time, in ticks (3 bytes)
TI_BANK         = 6           ; TASKINFO: its module's paged ROM bank ($FF: none)
TI_TYPE         = 7           ; TASKINFO: its module's type (HT_*; 0: none)
TI_NAME         = 8           ; TASKINFO: its name, zero-terminated (16 bytes)
TI_GROUP        = 24          ; TASKINFO: its note group
TI_SIZE         = 25          ; TASKINFO: the answer's size
NOTE_INTERRUPT  = 1           ; Notes: Ctrl-C (default: the end, 130)
NOTE_KILL       = 2           ; Notes: kill, never caught (137)
NOTE_HANGUP     = 3           ; Notes: hangup (129)
NOTE_ALARM      = 4           ; Notes: alarm (142)
NOTE_BRK        = 5           ; Notes: sys: brk, a BRK instruction (133)
NOTE_USER       = 16          ; Notes: 16-31 are the programs' own (128 + the note)
NOTE_GROUP      = $80         ; NOTE's .A: a note group, not a task
SEM_MUTEX       = $01         ; SEM_NEW's .X: a mutex (its count 1, given back only by its holder)
REBOOT_HWTEST   = $01         ; REBOOT's .A: into the hardware test
SEM_MAX         = 16          ; Semaphores: as many as there can be at once
SPAWN_NEWGROUP  = $01         ; SPAWN's flags: the child starts a note group of its own
SPAWN_NEWNS     = $02         ; SPAWN's flags: the child starts with an empty namespace of its own (else it shares its parent's)
SPAWN_FDMAP     = $04         ; SPAWN's flags: the child's fds are r2's map (else the caller's 0, 1 and 2)
SPAWN_NOENV     = $08         ; SPAWN's flags: the child starts with an empty environment (else a copy of its parent's)
SPAWN_STOPPED   = $10         ; SPAWN's flags: the child stops at its entry point, for the debugger (/proc/N/ctl's step)
ENV_SIZE        = 1024        ; TASKREAD's TR_ENV: its buffer, an environment's first variables, whole, and a 0
ENV_MAX         = 8192        ; An environment's bytes at most: each variable its name's length, its name, its value's length (2), its value; a 0
ENV_NAME_MAX    = 31          ; A variable's name's length at most
SPAWN_FDS       = 15          ; SPAWN's fd map: the child's fds it can give, at most (its fd 15 is its loader's as it starts)
MREPL           = 0           ; BIND's and MOUNT's flags: in place of what's at old
MBEFORE         = 1           ; BIND's and MOUNT's flags: a union, new first
MAFTER          = 2           ; BIND's and MOUNT's flags: a union, new last
MCREATE         = 4           ; BIND's and MOUNT's flags: a CREATE in the union comes to new
EXIT_MSG_MAX    = 31          ; An exit message's length at most (WAIT's buffer: 32 bytes)
KMESG_SIZE      = 4096        ; KMESG: the kernel's messages held at most (then the oldest go)
TR_ARGS         = 0           ; TASKREAD: a task's arguments (ARGS_MAX bytes)
TR_CWD          = 1           ; TASKREAD: a task's current directory (PATH_MAX + 1 bytes)
TR_ENV          = 2           ; TASKREAD: a task's environment's first variables, whole, and a 0 (ENV_SIZE bytes at most)
TR_FRAME        = 3           ; TASKREAD: a task's state and frame (TF_SIZE bytes)
TR_ENVAT        = 4           ; TASKREAD: part of a task's environment (r1 bytes at most, from offset r2; .A/.X its bytes in use)
TR_FD           = 5           ; TASKREAD: a task's fd r2 (FI_SIZE bytes)
FI_NAME         = 0           ; TR_FD's: the name its file was opened by (PATH_MAX + 1 bytes, zero-terminated) ...
FI_MODE         = 64          ; TR_FD's: its open mode (O_*) ...
FI_DEV          = 65          ; TR_FD's: its server's device letter ...
FI_QTYPE        = 66          ; TR_FD's: its qid type (QT_DIR ...) ...
FI_OFFSET       = 67          ; TR_FD's: and its offset (4 bytes, low first)
FI_SIZE         = 71          
TF_STATE        = 0           ; TR_FRAME's: the task's state (0 free, 1 ready, 2 waiting, 3 calling, 4 idle, 5 new, 6 sleeping, 7 blocked, 8 waiting for an event)
TF_S            = 1           ; TR_FRAME's: its stack pointer, as it was before the frame
TF_U            = 2           ; TR_FRAME's: the frame: U
TF_Y            = 3           
TF_W            = 4           
TF_X            = 5           
TF_A            = 6           
TF_P            = 7           
TF_PC           = 8           ; TR_FRAME's: PC (2)
TF_RAM          = 10          ; TR_FRAME's: its RAM bank register ($00)
TF_ROM          = 11          ; TR_FRAME's: its paged ROM bank register ($01)
TF_SIZE         = 12          
TM_READ         = 0           ; TASKMEM: from the task's memory
TM_WRITE        = $01         ; TASKMEM: to the task's memory
TM_BANK         = $80         ; TASKMEM's flag: its RAM bank r3, at $8000-$9FFF
TS_STEP         = 0           ; TASKSTEP: one instruction
TS_NEXT         = 1           ; TASKSTEP: one instruction, a JSR's subroutine whole
TS_BREAKS       = 2           ; TASKSTEP: from now, the task's BRKs stop it (the debugger's breakpoints)
TS_NOBREAKS     = 3           ; TASKSTEP: its BRKs are its own again (NOTE_BRK)
NI_FROM         = 0           ; NSINFO's answer (NI_SIZE bytes): the mount point (PATH_MAX + 1 bytes, zero-terminated)
NI_PATH         = 64          ; NSINFO: the path in the device (zero-terminated, no leading /)
NI_DEV          = 128         ; NSINFO: the device letter
NI_SPEC         = 129         ; NSINFO: the spec (8 bytes, zero-padded, and a 0)
NI_FLAGS        = 138         ; NSINFO: the flags (MCREATE)
NI_SEQ          = 139         ; NSINFO: its place among the union's members (lowest first)
NI_SIZE         = 140         
KEY_UP          = $80         ; Keys: a raw console read gives the terminal's cursor and function keys as one code each
KEY_DOWN        = $81         
KEY_RIGHT       = $82         
KEY_LEFT        = $83         
KEY_HOME        = $84         
KEY_END         = $85         
KEY_INS         = $86         
KEY_DEL         = $87         
KEY_PGUP        = $88         
KEY_PGDN        = $89         
KEY_F1          = $8A         ; Keys: F1-F12 are $8A-$95
KEY_F2          = $8B         
KEY_F3          = $8C         
KEY_F4          = $8D         
KEY_F5          = $8E         
KEY_F6          = $8F         
KEY_F7          = $90         
KEY_F8          = $91         
KEY_F9          = $92         
KEY_F10         = $93         
KEY_F11         = $94         
KEY_F12         = $95         
KEY_FOCUS       = $97         ; Keys: the focus moved to another window of this one's group; its number is the next key (a raw read's, keys hydra's)
KEY_RESIZE      = $96         ; Keys: the window's size changed (a raw read's, keys hydra's: consctl reads with the new one)
KEY_MOD         = $98         ; Keys: a key the terminal sent modified (keys mods's): the modifiers next (1 Shift, 2 Alt, 4 Ctrl), then the key
O_READ          = 0           ; Open modes
O_WRITE         = 1           
O_RDWR          = 2           
O_RW_MASK       = 3           
O_TRUNC         = $10         ; Open modes: empty the file first
O_NONBLOCK      = $40         ; Open modes: E_AGAIN rather than wait
FD_MAX          = 16          ; Fds a task has (0 stdin, 1 stdout, 2 stderr)
PATH_MAX        = 63          ; A path's length at most
IO_UNIT         = 512         ; The most a READ or WRITE asks of a server in one request
SR_NAME         = 0           ; The stat record (SR_SIZE bytes): its name (32, zero-terminated)
SR_QTYPE        = 32          ; Stat: the qid's type (QT_*)
SR_QVERS        = 33          ; Stat: the qid's version
SR_QPATH        = 34          ; Stat: the qid's path (4: unique in its server)
SR_MODE         = 38          ; Stat: the mode (2: permissions; high byte DM_DIR)
SR_LENGTH       = 40          ; Stat: the length (4)
SR_MTIME        = 44          ; Stat: the time it changed (4: seconds since 2000)
SR_DEV          = 48          ; Stat: the device letter
SR_INST         = 49          ; Stat: the device's instance (its spec's first character)
SR_SIZE         = 64          
QT_DIR          = $80         ; Qid types: a directory
QT_FILE         = $00         
DM_DIR          = $80         ; A mode's high byte: a directory
DM_APPEND       = $40         ; A mode's high byte: append-only (writes go to the end)
TASK_INBOX      = $0200       ; A server's copy of the request it's serving (RQ_*)
TASK_PATH       = $02C0       ; A server's copy of the name a request names (64 bytes)
RQ_TYPE         = 0           ; The request block (RQ_SIZE bytes): the request (R_*)
RQ_FID          = 1           ; Request: the fid (out: the new fid, R_OPEN and R_CREATE)
RQ_MODE         = 2           ; Request: the open mode
RQ_CLIENT       = 3           ; Request: the client task
RQ_U            = 4           ; Request: the client's U
RQ_FLAGS        = 5           ; Request: RF_*
RQ_OFFSET       = 6           ; Request: the offset (4)
RQ_COUNT        = 10          ; Request: the count asked for (2)
RQ_DONE         = 12          ; Request: the count done (2, out)
RQ_BUF          = 14          ; Request: the buffer, in the client's view (2)
RQ_PERM         = 16          ; Request: R_CREATE's new mode (high byte); out (R_OPEN, R_CREATE): the qid type
RQ_NAMELEN      = 17          ; Request: the name's length
RQ_GROUP        = 18          ; Request: the client's note group
RQ_DEV          = 19          ; Request: the device letter
RQ_SPEC         = 20          ; Request: the mount's spec (8, zero-padded)
RQ_EVENT        = 28          ; Request: the server's event count as it took it (after E_AGAIN, the client waits for a change)
RQ_SIZE         = 32          
RF_NONBLOCK     = $01         ; Request flags: answer E_AGAIN rather than wait
R_OPEN          = 1           ; Requests
R_CREATE        = 2           
R_READ          = 3           
R_WRITE         = 4           
R_CLUNK         = 5           
R_STAT          = 6           
R_WSTAT         = 7           
R_REMOVE        = 8           
R_FLUSH         = 9           
R_DUP           = 10          
SND_CHANNELS    = 24          ; Sound (#a): its channels: 0-7 the YM2151's, 8-23 the Vera X's PSG (its voices 0-15)
SND_PSG         = 8           ; Sound: the PSG's first channel (its voice 0)
SND_PATCHES     = 163         ; Sound: its patches (0-127: General MIDI's programs; 128-162: drums and percussion)
SND_DRUMS       = 128         ; Sound: General MIDI's drums (MIDI channel 10's notes)
SND_R_CH        = $02         ; Sound: a command, at a register the chip doesn't have: the channel the next commands are for (0-23)
SND_R_PATCH     = $03         ; Sound: load patch n into the channel (its speakers stay)
SND_R_NOTE      = $04         ; Sound: key on MIDI note n (60: middle C; 69: A, 440 Hz), with the channel's bend
SND_R_OFF       = $05         ; Sound: key off (the release)
SND_R_VOL       = $06         ; Sound: the channel's volume (0-127, as General MIDI's)
SND_R_PAN       = $07         ; Sound: the channel's speakers (SND_PAN_*)
SND_R_BEND      = $09         ; Sound: the channel's bend (signed: 64ths of a semitone; the note playing too)
SND_R_DRUM      = $0A         ; Sound: a General MIDI drum (its patch and pitch), keyed on
SND_R_FREQ_LO   = $0B         ; Sound: a frequency's low byte (Hz), for SND_R_FREQ (only kept)
SND_R_FREQ      = $0C         ; Sound: key on at a frequency, SND_R_FREQ_LO's byte and this its high one (Hz; 0: key off)
SND_R_GLIDE     = $0D         ; Sound: the channel's pitch to note n without a new attack (legato)
SND_R_WAVE      = $0E         ; Sound: a PSG channel's waveform: SND_WAVE_* << 6 | its pulse width (0-63: 63 a square)
SND_PAN_LEFT    = 1           ; Sound: SND_R_PAN's speakers
SND_PAN_RIGHT   = 2           
SND_PAN_BOTH    = 3           
SND_WAVE_PULSE  = 0           ; Sound: SND_R_WAVE's waveforms (the PSG's)
SND_WAVE_SAW    = 1           
SND_WAVE_TRIANGLE = 2           
SND_WAVE_NOISE  = 3           
r0              = $02         ; The call registers r0-r15: arguments and results
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
