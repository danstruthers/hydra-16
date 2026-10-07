; ****************************************************************************
; hydra.s - the Hydra's own calls for C (include/hydra.h): semaphores, the scheduler's ticks, sleeping and
; yielding, the clock, the task's number and waiting for another's end; sleep() on them; and the helpers system.c
; uses (a pipe, fds, the command shell).  A failed call sets _oserror and errno, and returns -1.

        .export     _hy_sem_new, _hy_mutex_new, _hy_sem_acquire, _hy_sem_try, _hy_sem_release, _hy_sem_free
        .export     _hy_ticks, _hy_sleep_ticks, _hy_yield, _hy_clock, _hy_task, _hy_wait, _hy_kill, _sleep
        .export     __hy_pipe, __hy_dup, __hy_dup2, __hy_shell
        .import     ___mappederrno, popax

        .include    "zeropage.inc"
        .include    "hydra.inc"

        .code

; int __fastcall__ hy_sem_new (unsigned char count): a semaphore (1-16)
_hy_sem_new:
        ldy         #0
        bra         semnew

; int hy_mutex_new (void): a mutex (a semaphore of 1 with a holder)
_hy_mutex_new:
        lda         #1
        ldy         #SEM_MUTEX
semnew:
        jsr         SEM_NEW
        bcs         error
        ldx         #0
        rts

error:
        jmp         ___mappederrno

; int __fastcall__ hy_sem_acquire (unsigned char s): take one, waiting until there's one
_hy_sem_acquire:
        jsr         SEM_ACQUIRE
        bra         zero

; int __fastcall__ hy_sem_release (unsigned char s): give one back
_hy_sem_release:
        jsr         SEM_RELEASE
        bra         zero

; int __fastcall__ hy_sem_free (unsigned char s)
_hy_sem_free:
        jsr         SEM_FREE
zero:
        bcs         error
        lda         #0
        tax
        rts

; int __fastcall__ hy_sem_try (unsigned char s): 1: taken; 0: none (at once)
_hy_sem_try:
        jsr         SEM_TRY
        bcc         @taken
        cmp         #ERR_SEM_BUSY
        bne         error
        lda         #0
        tax
        rts

@taken:
        lda         #1
        ldx         #0
        rts

; unsigned hy_ticks (void): the scheduler's tick count (HY_TICKS_PER_SEC a second; it wraps)
_hy_ticks:
        jsr         TICKS_GET
        pha
        tya
        tax
        pla
        rts

; void __fastcall__ hy_sleep_ticks (unsigned ticks): sleep (up to 32767 ticks); the other tasks run
_hy_sleep_ticks:
        pha
        txa
        tay
        pla
        jmp         TASK_SLEEP

; void hy_yield (void): let the other tasks run
_hy_yield:
        jmp         YIELD

; unsigned long hy_clock (void): the Hydra's clock, seconds since 2000-01-01 00:00:00
_hy_clock:
        ldx         #tmp1                               ; (tmp1-tmp4: four bytes in a row)
        jsr         CLOCK_GET
        lda         tmp3
        sta         sreg
        lda         tmp4
        sta         sreg + 1
        lda         tmp1
        ldx         tmp2
        rts
.assert     tmp2 = tmp1 + 1 .and tmp3 = tmp1 + 2 .and tmp4 = tmp1 + 3, error, "_hy_clock: tmp1-tmp4 in a row"

; unsigned char hy_task (void): this task's number (1-15; as ps shows it)
_hy_task:
        lda         T_REGISTER
        and         #$0F
        ldx         #0
        rts

; int __fastcall__ hy_kill (int task): end task and the tasks it started, as the kill word and Ctrl-\ do (its
; exit status: 137, "killed")
_hy_kill:
        tax
        lda         #TASK_KILL_FLAG
        jsr         TASK_SIGNAL
        bra         zero

; int __fastcall__ hy_wait (int task, char* msg): wait for task (one this one started) to end: its exit status's
; code (0-255), and its message (up to HY_STATUS_MAX - 1 characters) into msg, if it's not NULL
_hy_wait:
        sta         ZP_IO_BUF
        stx         ZP_IO_BUF + 1                       ; (NULL: a high byte of 0, no message wanted)
        jsr         popax                               ; The task
        jsr         TASK_JOIN
        bcs         error2
        ldx         #0
        rts

error2:
        jmp         ___mappederrno                      ; (Near the routines after here)

; unsigned __fastcall__ sleep (unsigned seconds): 0 (it slept them all; Ctrl-C ends the program)
_sleep:
        sta         ptr1
        stx         ptr1 + 1

@second:
        lda         ptr1
        ora         ptr1 + 1
        beq         @done
        lda         #<TICKS_PER_SEC
        ldy         #>TICKS_PER_SEC
        jsr         TASK_SLEEP
        lda         ptr1
        bne         :+
        dec         ptr1 + 1
:
        dec         ptr1
        bra         @second

@done:
        lda         #0
        tax
        rts

; int __fastcall__ _hy_pipe (int* fds): a pipe: fds[0] its reading end, fds[1] its writing end.  0, or -1
__hy_pipe:
        sta         ptr1
        stx         ptr1 + 1
        jsr         IO_PIPE
        bcs         error2
        ldy         #0
        sta         (ptr1),y
        tya
        iny
        sta         (ptr1),y
        txa
        iny
        sta         (ptr1),y
        lda         #0
        iny
        sta         (ptr1),y
        tax
        rts

; int __fastcall__ _hy_dup (int fd): another fd for its file, or -1
__hy_dup:
        jsr         IO_DUP
        bcs         error2
        ldx         #0
        rts

; int __fastcall__ _hy_dup2 (int fd, int newfd): newfd is fd's file too (closed first if it was open).  0, or -1
__hy_dup2:
        sta         tmp1                                ; newfd
        jsr         popax                               ; fd
        ldx         tmp1
        jsr         IO_DUP2
        bcs         error2
        lda         #0
        tax
        rts

; int _hy_shell (void): a command shell (SHELL_CMD: HyForth running its stdin's lines) in a new task, with this
; task's fds (as they are now), namespace, current directory and environment: its task, or -1
__hy_shell:
        lda         #<SHELL_CMD
        ldy         #>SHELL_CMD
        ldx         #0                                  ; (ROM page 0: the thunk)
        jsr         TASK_RUN
        bcs         error2
        ldx         #0
        rts
