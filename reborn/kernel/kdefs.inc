; ****************************************************************************
; kernel/kdefs.inc - what the kernel's files share: the layout (include/layout.inc), the error codes (made from
; spec/errors.def), every routine one kernel file calls in another (.global: an export where it's defined, an
; import elsewhere, so the files are assembled on their own), and the macros for reaching other tasks.

.include "layout.inc"
.include "errors.inc"

.setcpu "65C02"

; ---- reset.s: boot
.global     BOOT, BOOT_DONE, RESET_ENTRY
; ---- common.s: the COMMON block (the same on every BIOS ROM page)
.global     IRQ_ENTRY, IRQ_RESTORE, IRQ_EXIT, NMI_ENTRY
.global     IRQ_STUB_0, IRQ_STUB_F, IRQ_VIA, K_FAR, K_FAR_GO, K_PEEK_PAGE
; ---- irq.s: interrupts
.global     IRQ_HANDLER, IRQ_SWITCH, IRQ_STRAY, IRQ_INIT, IRQ_SET_OWNER, IRQ_RELEASE_ALL, NMI_HANDLER, K_KIRQ
; ---- sched.s: the scheduler, waiting, time
.global     K_SCHED_SWITCH, K_SCHED_RESUME, K_YIELD, K_PAUSE, K_WAKE, K_EVWAIT, K_PREEMPT_OFF, K_PREEMPT_ON
.global     K_SLEEP, K_SLEEP_UNTIL, K_TICKS
; ---- call.s: calls into other tasks, and kcopy
.global     K_SCALL, K_KCALL, K_KDISPATCH, K_KCOPY, K_ROMREAD
; ---- task.s: tasks and modules
.global     K_EXITS, K_WAIT, K_GETPID, K_GETPPID,K_TASK_BOOT, K_TASK_BOOT_INIT, K_BOOT_STARTED, K_TASK_RESERVE
.global     K_MD_VALID, K_STR_CRLF, K_MODINFO, K_SPAWN_K, K_TASK_GO, K_TASK_LOAD
; ---- load.s: SPAWN and the loader (page 3)
.global     K_SPAWN, K_LOAD
; ---- cons.s: the bring-up console (polled)
.global     K_CONS_INIT, K_PUTC, K_PUTS, K_GETC, K_PUTHEX, K_PUTNIB, K_PUTSTR
; ---- sys.s: the small calls
.global     K_SYSINFO, K_ERRSTR, K_NOSYS, K_IRQ_OWN, K_IRQ_RELEASE, K_DBG_KCOPY
; ---- mem.s: memory (page 1)
.global     K_MEM_START, K_SEG_EXIT, K_BRK, K_PAGES_ALLOC, K_PAGES_FREE, K_BANKS, K_BANKS_ALLOC, K_BANKS_FREE
.global     K_SEG_CREATE, K_SEG_ATTACH, K_SEG_DETACH, K_SEG_MAP, K_SEGINFO
; ---- sem.s: semaphores (page 1)
.global     K_SEM_NEW, K_SEM_ACQUIRE, K_SEM_TRY, K_SEM_RELEASE, K_SEM_FREE, K_SEM_EXIT
; ---- notes.s: notes (signals)
.global     K_NOTE, K_NOTE_POST, K_NOTE_QUEUE, K_NOTE_QUEUED, K_UPAUSE, K_NOTE_CHECK, K_NOTE_RETURN, K_NOTE_TRAMP, K_NOTIFY
; ---- file.s: files (page 2)
.global     K_OPEN, K_CREATE, K_CLOSE, K_CLOSE_ALL, K_DUP, K_DUP2, K_READ, K_WRITE, K_SEEK, K_FSTAT, K_FWSTAT
.global     K_STAT, K_WSTAT, K_REMOVE, K_SRV_REGISTER, K_SRV_TAKE, K_SRV_REPLY, K_CLIENT_READ, K_CLIENT_WRITE
.global     K_FILE_EXIT, K_FD_INHERIT, K_CHDIR, K_GETCWD, K_PIPE, K_BIND, K_MOUNT, K_UNMOUNT, K_DEV_FIND_K
; ---- ns.s: namespaces (page 3)
.global     K_NS_INIT, K_NS_FRESH, K_NS_INHERIT, K_NS_EXIT, K_NS_FIND_K, K_NS_NEXT_K, K_NS_CWD_K, K_NS_UNION_K
.global     K_NS_UNEXT_K, K_NS_UREL_K, K_NS_BIND_K, K_NS_MOUNT_K, K_NS_UNMOUNT_K, K_NSINFO
; ---- env.s: environments (page 2)
.global     K_ENV_GET, K_ENV_PUT, K_ENV_DEL, K_ENV_NAME, K_ENV_INHERIT, K_ENV_CLEAR, K_ENV_INIT, K_ENV_READ
; ---- info.s: what the kernel knows of the tasks (page 1)
.global     K_TASKINFO, K_DBG_PS, K_TASKREAD, K_TASKMEM, K_TASKSTOP, K_FD2PATH, K_KMESG
; ---- time.s: the clock, and the DS1747 (page 1)
.global     K_TIME, K_TIME_SET, K_RTC
; ---- far.s: the jump table's way to calls on other pages
.global     K_FARJMP
; ---- post.s: the power-on self test (page 4)
.global     K_POST
; ---- errtext.s (made from spec/errors.def)
.global     ERR_TEXTS

; ---- Quick looks: a moment in another task's zero page or RAM, with IRQs off and no stack (the stack page
; changes with T).  These are the only places T is written (and the IRQ path, the scheduler, SCALL and kcopy).

; .A = task .X's byte at addr.  IN: IRQs off, .Y = this task.  Keeps .X, .Y
.macro QL_GET   addr
            stx         T_REGISTER
            lda         addr
            sty         T_REGISTER
.endmacro

; Task .X's byte at addr = .A.  IN: IRQs off, .Y = this task.  Keeps .A, .X, .Y
.macro QL_PUT   addr
            stx         T_REGISTER
            sta         addr
            sty         T_REGISTER
.endmacro

; .A = the kernel task's byte at addr (a quick look).  IN: IRQs off, .Y = this task.  Keeps .X, .Y
.macro K0_GET   addr
            stz         T_REGISTER
            lda         addr
            sty         T_REGISTER
.endmacro

; The kernel task's byte at addr = .A.  IN: IRQs off, .Y = this task.  Keeps .A, .X, .Y
.macro K0_PUT   addr
            stz         T_REGISTER
            sta         addr
            sty         T_REGISTER
.endmacro

; Run routine (on any BIOS ROM page) in the kernel task, one KCALL at a time (call.s: K_KCALL); the kernel task
; is never preempted, so the routine runs to its end.  It gets .A and .X as they are here, and .Y = the calling
; task; it gives back .A, .X and C.  The routine is named in the caller's own zero page (K_FNVEC, K_FNPAGE), where
; the kernel task reads it.  KCALL from page 0, KCALL_FAR from the others
.macro KCALL    routine
            KCALL_NAME  routine
            jsr         K_KCALL
.endmacro

.macro KCALL_FAR routine
            KCALL_NAME  routine
            FARCALL     K_KCALL
.endmacro

.macro KCALL_NAME routine
            pha
            lda         #<routine
            sta         K_FNVEC
            lda         #>routine
            sta         K_FNVEC + 1
            lda         #<.bank(routine)
            sta         K_FNPAGE
            pla
.endmacro

; Call routine on whichever BIOS ROM page it's on (its bank in bios.cfg), and come back to this one
; (common.s: K_FAR).  .A, .X, .Y and C pass both ways; N and Z don't.  About 60 cycles more than a jsr
.macro FARCALL  routine
            pha
            lda         #<.bank(routine)
            sta         KF_PAGE
            lda         #<routine
            sta         KF_VEC
            lda         #>routine
            sta         KF_VEC + 1
            pla
            jsr         K_FAR
.endmacro

; Print a string on the bring-up console (cons.s: K_PUTSTR).  Keeps .X, .Y
.macro KPRINT   label
            pha
            lda         #<label
            sta         r0
            lda         #>label
            sta         r0 + 1
            pla
            jsr         K_PUTSTR
.endmacro

; Fail with an error code: C = 1, .A = code, and return
.macro FAIL     code
            lda         #code
            sec
            rts
.endmacro

CR              = $0D
LF              = $0A

; ---- More of the kernel task's RAM
K_NAMEBUF       = $0700         ; (16) A module's name being looked for (SPAWN)
K_STRBUF        = $0710         ; (32) A string from another task (an exit message)
