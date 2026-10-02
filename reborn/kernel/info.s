; ****************************************************************************
; info.s - what the kernel knows of the tasks (BIOS ROM page 1: far calls).  TASKINFO for programs (ps; /proc's
; source when it comes), and DBG_PS, a table of them all on the console.
;
; Another task's bytes are read with quick looks, 4 at a time with IRQs off, not with kcopy: kcopy keeps its
; pointer in its partner's zero page, and a task chosen at random may be in the middle of a kcopy of its own.

.include "kdefs.inc"

.segment "KCODE_P1"

; TASKINFO: what the kernel knows of a task.  IN: .A = the task; r0 = a buffer (TI_SIZE bytes).  OUT: the buffer
; filled (TI_*: layout.inc); C = 0; or C = 1, .A = E_SRCH (not a task).  Its staging: this task's TA_SCRATCH
K_TASKINFO:
            cmp         #TASKS
            bcc         :+
            FAIL        E_SRCH
:
            tax                                             ; .X = the task, .Y = this one, throughout
            ldy         T_REGISTER
            php
            sei
            stx         T_REGISTER                          ; ---- The task: its state and flags
            lda         TK_STATE
            sty         T_REGISTER                          ; ---- Back
            sta         TA_SCRATCH + TI_STATE
            stx         T_REGISTER                          ; ---- The task
            lda         TK_FLAGS
            sty         T_REGISTER                          ; ---- Back
            sta         TA_SCRATCH + TI_FLAGS
            plp
            php
            sei
            stz         T_REGISTER                          ; ---- The kernel task's tables: its CPU time (at once:
            lda         K_CPU_LO,X                          ;   the tick changes it)
            sty         T_REGISTER                          ; ---- Back
            sta         TA_SCRATCH + TI_CPU
            stz         T_REGISTER
            lda         K_CPU_MID,X
            sty         T_REGISTER
            sta         TA_SCRATCH + TI_CPU + 1
            stz         T_REGISTER
            lda         K_CPU_HI,X
            sty         T_REGISTER
            sta         TA_SCRATCH + TI_CPU + 2
            plp
            php
            sei
            stz         T_REGISTER                          ; ---- Its parent, its module
            lda         K_PARENT,X
            sty         T_REGISTER
            sta         TA_SCRATCH + TI_PARENT
            stz         T_REGISTER
            lda         K_TASK_BANK,X
            sty         T_REGISTER
            sta         TA_SCRATCH + TI_BANK
            stz         T_REGISTER
            lda         K_TASK_TYPE,X
            sty         T_REGISTER
            sta         TA_SCRATCH + TI_TYPE
            stz         T_REGISTER
            lda         K_NGROUP,X
            sty         T_REGISTER
            sta         TA_SCRATCH + TI_GROUP
            plp
.repeat 4, I                                                ; Its name, 4 bytes at a time
            php
            sei
    .repeat 4, J
            stx         T_REGISTER                          ; ---- The task
            lda         TA_NAME + I * 4 + J
            sty         T_REGISTER                          ; ---- Back
            sta         TA_SCRATCH + TI_NAME + I * 4 + J
    .endrepeat
            plp
.endrepeat
            stz         TA_SCRATCH + TI_NAME + 15           ; (Zero-terminated, whatever it holds)
            ldy         #TI_SIZE - 1                        ; To the buffer
:
            lda         TA_SCRATCH,Y
            sta         (r0),Y
            dey
            bpl         :-
            clc
            rts

; DBG_PS: a line for each task in use: "T ST FL PA CPU    NAME" (its number, state, flags, parent, CPU time in
; ticks, all hex; its name).  Its scratch: K_TASK, and TA_SCRATCH + PS_INFO for TASKINFO's answers
PS_INFO         = 32

K_DBG_PS:
            ldx         #PS_S_HEAD - PS_STRINGS
            jsr         ps_puts
            stz         K_TASK
@task:
            lda         #<(TA_SCRATCH + PS_INFO)
            sta         r0
            lda         #>(TA_SCRATCH + PS_INFO)
            sta         r0 + 1
            lda         K_TASK
            jsr         K_TASKINFO
            lda         K_TASK
            beq         :+                                  ; (The kernel task: always)
            lda         TA_SCRATCH + PS_INFO + TI_STATE
            beq         @next                               ; (Free)
:
            lda         K_TASK
            jsr         ps_putnib
            lda         TA_SCRATCH + PS_INFO + TI_STATE
            jsr         @hex
            lda         TA_SCRATCH + PS_INFO + TI_FLAGS
            jsr         @hex
            lda         TA_SCRATCH + PS_INFO + TI_PARENT
            jsr         @hex
            lda         #' '
            jsr         ps_putc
            lda         TA_SCRATCH + PS_INFO + TI_CPU + 2
            jsr         ps_puthex
            lda         TA_SCRATCH + PS_INFO + TI_CPU + 1
            jsr         ps_puthex
            lda         TA_SCRATCH + PS_INFO + TI_CPU
            jsr         ps_puthex
            lda         #' '
            jsr         ps_putc
            lda         #<(TA_SCRATCH + PS_INFO + TI_NAME)  ; (In RAM: page 0's routine can read it)
            sta         r0
            lda         #>(TA_SCRATCH + PS_INFO + TI_NAME)
            sta         r0 + 1
            jsr         ps_putstr
            ldx         #PS_S_CRLF - PS_STRINGS
            jsr         ps_puts
@next:
            inc         K_TASK
            lda         K_TASK
            cmp         #TASKS
            bne         @task
            clc
            rts

@hex:                                                       ; " xx"
            pha
            lda         #' '
            jsr         ps_putc
            pla
            jsr         ps_puthex
            rts

; The string at offset .X in PS_STRINGS (on this page: a character at a time).  Modifies .A, .X
ps_puts:
            lda         PS_STRINGS,X
            beq         :+
            jsr         ps_putc
            inx
            bra         ps_puts
:
            rts

; Page 0's console routines, from here
ps_putc:
            FARCALL     K_PUTC
            rts

ps_puthex:
            FARCALL     K_PUTHEX
            rts

ps_putnib:
            FARCALL     K_PUTNIB
            rts

ps_putstr:
            FARCALL     K_PUTSTR
            rts

.segment "KRODATA_P1"
PS_STRINGS:
PS_S_HEAD:  .byte       "T ST FL PA CPU    NAME"
PS_S_CRLF:  .byte       CR, LF, 0
