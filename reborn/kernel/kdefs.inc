; ****************************************************************************
; kernel/kdefs.inc - what the kernel's files share: the layout (include/layout.inc), the error codes (made from
; spec/errors.def), every routine one kernel file calls in another (.global: an export where it's defined, an
; import elsewhere, so the files are assembled on their own), and the macros for reaching other tasks.

.include "layout.inc"
.include "errors.inc"

.setcpu "65C02"

; ---- reset.s: boot
.global     BOOT, RESET_ENTRY
; ---- common.s: the COMMON block (the same on every BIOS ROM page)
.global     IRQ_ENTRY, IRQ_EXIT, NMI_ENTRY
.global     IRQ_STUB_0, IRQ_STUB_F
; ---- irq.s: interrupts
.global     IRQ_DISPATCH, IRQ_INIT, IRQ_SET_OWNER, IRQ_RELEASE_ALL, NMI_HANDLER, K_KIRQ
; ---- sched.s: the scheduler, waiting, time
.global     K_SCHED_SWITCH, K_SCHED_RESUME, K_YIELD, K_PAUSE, K_WAKE, K_PREEMPT_OFF, K_PREEMPT_ON
.global     K_SLEEP, K_SLEEP_UNTIL, K_TICKS
; ---- call.s: calls into other tasks, and kcopy
.global     K_SCALL, K_KCALL, K_KDISPATCH, K_KCOPY
; ---- task.s: tasks and modules
.global     K_SPAWN, K_EXITS, K_WAIT, K_GETPID, K_TASK_BOOT, K_TASK_RESERVE
; ---- cons.s: the bring-up console (polled)
.global     K_CONS_INIT, K_PUTC, K_PUTS, K_GETC, K_PUTHEX, K_PUTNIB, K_PUTSTR
; ---- sys.s: the small calls
.global     K_SYSINFO, K_ERRSTR, K_NOSYS, K_IRQ_OWN, K_IRQ_RELEASE, K_DBG_KCOPY, K_DBG_PS
.global     K_PROBE
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

; Run routine in the kernel task, one KCALL at a time (call.s: K_KCALL); the kernel task is never preempted, so
; the routine runs to its end.  It gets .A and .X as they are here, and .Y = the calling task; it gives back .A,
; .X and C.  IRQs must be on: they stay off from naming the routine until the kernel task runs it (so no other
; KCALL can name another meanwhile), and are on again after.
.macro KCALL    routine
            pha
            phx
            ldx         T_REGISTER
            sei
            lda         #<routine
            stz         T_REGISTER
            sta         K0_FNVEC
            lda         #>routine
            sta         K0_FNVEC + 1
            stx         T_REGISTER
            plx
            pla
            jsr         K_KCALL
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

; ---- More of the per-task OS area: a call stub's scratch (64 bytes), for what a call copies in or out
TA_SCRATCH      = $0280

; ---- More of the kernel task's RAM
K_NAMEBUF       = $0700         ; (16) A module's name being looked for (SPAWN)
K_STRBUF        = $0710         ; (32) A string from another task (an exit message)
