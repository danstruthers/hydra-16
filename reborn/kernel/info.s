; ****************************************************************************
; info.s - what the kernel knows of the tasks (BIOS ROM page 1: far calls).  TASKINFO and TASKREAD for programs
; (ps; /proc's source), and DBG_PS, a table of them all on the console.
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

; TASKREAD: a task's arguments, current directory, environment, or its state and frame.  IN: .A = a task ($FF:
; this one); .X = TR_ARGS, TR_CWD, TR_ENV or TR_FRAME; r0 = a buffer (TA_ARGS_MAX + 1, PATH_MAX + 1, ENV_SIZE or
; TF_SIZE bytes).  OUT: the bytes in it; or C = 1, .A = E_SRCH, E_INVAL
K_TASKREAD:
            cpx         #TR_FRAME + 1
            bcc         :+
            FAIL        E_INVAL

:
            KCALL_FAR   K_TASKREAD_K
            rts

; TASKREAD's (a KCALL: .Y = the caller): the bytes into K_XBUF, a byte at a time with IRQs off for each (the
; task's OS area, read with T switched to it: an index register free, as T comes back to 0), then to the caller
K_TASKREAD_K:
            sty         K0_TMP2                             ; (The caller)
            stx         K0_TMP3                             ; (What)
            cmp         #$FF
            bne         :+
            tya
:
            cmp         #TASKS
            bcs         @srch
            tax
            ldy         T_REGISTER
            php
            sei
            QL_GET      TK_STATE
            plp
            cmp         #ST_FREE
            beq         @srch
            ldy         #0
            lda         K0_TMP3
            cmp         #TR_ENV
            bcc         :++
            beq         :+
            jmp         @frame
:
            jmp         @env
:
            cmp         #TR_CWD
            beq         @cwd
@args:
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            lda         TA_ARGS,Y
            stz         T_REGISTER                          ; ---- Back
            plp
            sta         K_XBUF,Y
            iny
            cpy         #TA_ARGS_MAX + 1
            bne         @args
            bra         @copy

@srch:
            FAIL        E_SRCH

@cwd:
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            lda         TA_CWD,Y
            stz         T_REGISTER                          ; ---- Back
            plp
            sta         K_XBUF,Y
            iny
            cpy         #PATH_MAX + 1
            bne         @cwd
@copy:
            sty         K_CNT                               ; To the caller's buffer
            stz         K_CNT + 1
            lda         #<K_XBUF
            sta         K_PTR
            lda         #>K_XBUF
            sta         K_PTR + 1
@kcopy:
            ldx         K0_TMP2
            ldy         T_REGISTER
            php
            sei
            QL_GET      r0
            sta         K_PTR2
            QL_GET      r0 + 1
            sta         K_PTR2 + 1
            plp
            lda         K0_TMP2
            clc
            FARCALL     K_KCOPY
            clc
            rts

@env:                                                       ; Its environment: the kernel task's own (K_ENV)
            txa
            asl
            asl
            clc
            adc         #>K_ENV
            sta         K_PTR + 1
            lda         #<K_ENV
            sta         K_PTR
            lda         #<ENV_SIZE
            sta         K_CNT
            lda         #>ENV_SIZE
            sta         K_CNT + 1
            bra         @kcopy

@frame:                                                     ; Its state, and the frame it left at TK_SP (on its
            php                                             ;   stack: its own stack page), and its bank registers
            sei                                             ;   (their mirrors)
            stx         T_REGISTER                          ; ---- The task
            lda         TK_STATE
            ldy         TK_SP
            stz         T_REGISTER                          ; ---- Back
            plp
            sta         K_XBUF + TF_STATE
            tya
            clc
            adc         #FRAME_SIZE
            sta         K_XBUF + TF_S
.repeat     FRAME_SIZE, I
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            lda         $0100 + FR_U + I,Y
            stz         T_REGISTER                          ; ---- Back
            plp
            sta         K_XBUF + TF_U + I
.endrepeat
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            lda         RAM_BANK
            ldy         ROM_BANK
            stz         T_REGISTER                          ; ---- Back
            plp
            sta         K_XBUF + TF_RAM
            sty         K_XBUF + TF_ROM
            ldy         #TF_SIZE
            jmp         @copy

.assert     ENV_SIZE = $400 .and <K_ENV = 0, error, "TASKREAD's TR_ENV: 4 pages a task, from a page"
.assert     TF_Y - TF_U = FR_Y - FR_U .and TF_PC - TF_U = FR_PCL - FR_U .and TF_RAM = TF_U + FRAME_SIZE, error, "TF_* and FR_*"

; TASKMEM: bytes between a task's memory, as it sees it, and a buffer here (/proc/N/mem and ram).  IN: .A = the
; task; .X = TM_READ or TM_WRITE, | TM_BANK; r0 = the buffer; r1 = the address in the task's view, $0000-$DFFF
; (TM_BANK: $8000-$9FFF, of its RAM bank r3); r2 = the count.  OUT: C = 0; or C = 1, .A = E_PERM (the caller isn't
; a driver: a program reaches another task's memory only through /proc, whose server decides who may), E_SRCH,
; E_INVAL
;   In the caller's task.  A byte at a time, IRQs off for each, T switched to the task and back: TM_PTR is each end's
; pointer and TM_PARTNER the other task (as kcopy's KC_PTR and KC_PARTNER: TASKMEM's own, so a task part-way through a
; kcopy is no matter).  The task's $8000-$DFFF are its bank and paged ROM as it has them selected (T's); with TM_BANK
; its bank register points at bank r3 from the first byte to the last (it isn't running: only an irq entry of its own
; could see it, and the caller isn't to ask it of a driver).  Modifies .A, .X, .Y, K_TASK, K_TMP, K_PTR, K_CNT
K_TASKMEM:
            tay                                             ; (A driver's call only)
            lda         TK_FLAGS
            and         #TF_DRIVER
            bne         :+
            FAIL        E_PERM

:
            tya
            cmp         #TASKS
            bcs         @srch
            sta         K_TASK
            stx         K_TMP
            clc                                             ; The range's end (past it): $E000 at most; with
            lda         r1                                  ;   TM_BANK, from $8000 to $A000
            adc         r2
            sta         K_PTR
            lda         r1 + 1
            adc         r2 + 1
            sta         K_PTR + 1
            bcs         @inval
            ldx         #>BIOS_BASE
            bit         K_TMP
            bpl         :+
            lda         r1 + 1
            cmp         #>BANK_WINDOW
            bcc         @inval
            ldx         #>(BANK_WINDOW + BANK_SIZE)
:
            stx         K_CNT
            lda         K_PTR + 1                           ; (The end at or below it)
            cmp         K_CNT
            bcc         @range
            bne         @inval
            lda         K_PTR
            bne         @inval
@range:
            ldx         K_TASK
            ldy         T_REGISTER
            php
            sei
            QL_GET      TK_STATE
            cmp         #ST_FREE
            bne         :+
            plp
@srch:
            FAIL        E_SRCH

@inval:
            FAIL        E_INVAL

:
            lda         r1                                  ; Its end, and its partner: us
            QL_PUT      TM_PTR
            lda         r1 + 1
            QL_PUT      TM_PTR + 1
            tya
            QL_PUT      TM_PARTNER
            lda         r0                                  ; Ours
            sta         TM_PTR
            lda         r0 + 1
            sta         TM_PTR + 1
            stx         TM_PARTNER
            bit         K_TMP                               ; TM_BANK: its bank register at bank r3 (its own kept
            bpl         :+                                  ;   in its TM_OLDBANK)
            lda         r3
            stx         T_REGISTER                          ; ---- The task
            ldx         RAM_BANK
            stx         TM_OLDBANK
            sta         RAM_BANK
            sty         T_REGISTER                          ; ---- Back
:
            plp
            lda         r2
            sta         K_CNT
            lda         r2 + 1
            sta         K_CNT + 1
            ldy         #0
            lda         K_TMP
            lsr                                             ; (TM_WRITE)
            bcs         @out
@in:                                                        ; ---- Its to ours
            lda         K_CNT
            ora         K_CNT + 1
            beq         @done
            php
            sei
            ldx         TM_PARTNER
            stx         T_REGISTER                          ; ---- The task
            lda         (TM_PTR),Y
            ldx         TM_PARTNER
            stx         T_REGISTER                          ; ---- Back
            sta         (TM_PTR),Y
            plp
            jsr         tm_next
            bra         @in

@out:                                                       ; ---- Ours to its
            lda         K_CNT
            ora         K_CNT + 1
            beq         @done
            php
            sei
            lda         (TM_PTR),Y
            ldx         TM_PARTNER
            stx         T_REGISTER                          ; ---- The task
            sta         (TM_PTR),Y
            ldx         TM_PARTNER
            stx         T_REGISTER                          ; ---- Back
            plp
            jsr         tm_next
            bra         @out

@done:
            bit         K_TMP                               ; TM_BANK: its own bank back
            bpl         :+
            ldx         K_TASK
            ldy         T_REGISTER
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            lda         TM_OLDBANK
            sta         RAM_BANK
            sty         T_REGISTER                          ; ---- Back
            plp
:
            clc
            rts

; TASKMEM's next byte: .Y on (a page past: both ends' pointers on a page), K_CNT down.  Keeps .Y's meaning
tm_next:
            iny
            bne         :+
            inc         TM_PTR + 1                          ; (Ours ...
            php
            sei
            ldx         TM_PARTNER
            stx         T_REGISTER                          ; ---- The task
            inc         TM_PTR + 1                          ;   ... and its)
            ldx         TM_PARTNER
            stx         T_REGISTER                          ; ---- Back
            plp
:
            lda         K_CNT
            bne         :+
            dec         K_CNT + 1
:
            dec         K_CNT
            rts

; DBG_PS: a line for each task in use: "T ST FL PA CPU    NAME" (its number, state, flags, parent, CPU time in
; ticks, all hex; its name).  Its scratch: K_TASK, and PS_INFO (its TA_PATH: it makes no request) for
; TASKINFO's answers
PS_INFO         = TA_PATH

K_DBG_PS:
            ldx         #PS_S_HEAD - PS_STRINGS
            jsr         ps_puts
            stz         K_TASK
@task:
            lda         #<(PS_INFO)
            sta         r0
            lda         #>(PS_INFO)
            sta         r0 + 1
            lda         K_TASK
            jsr         K_TASKINFO
            lda         K_TASK
            beq         :+                                  ; (The kernel task: always)
            lda         PS_INFO + TI_STATE
            beq         @next                               ; (Free)
:
            lda         K_TASK
            jsr         ps_putnib
            lda         PS_INFO + TI_STATE
            jsr         @hex
            lda         PS_INFO + TI_FLAGS
            jsr         @hex
            lda         PS_INFO + TI_PARENT
            jsr         @hex
            lda         #' '
            jsr         ps_putc
            lda         PS_INFO + TI_CPU + 2
            jsr         ps_puthex
            lda         PS_INFO + TI_CPU + 1
            jsr         ps_puthex
            lda         PS_INFO + TI_CPU
            jsr         ps_puthex
            lda         #' '
            jsr         ps_putc
            lda         #<(PS_INFO + TI_NAME)  ; (In RAM: page 0's routine can read it)
            sta         r0
            lda         #>(PS_INFO + TI_NAME)
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
