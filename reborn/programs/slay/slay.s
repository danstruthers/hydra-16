; ****************************************************************************
; slay [-i] name ... - each task running a program of that name ended (NOTE_KILL); -i: interrupted instead
; (NOTE_INTERRUPT).  Not slay itself.  A name no task has is said ("slay: name: no such task"), and slay ends with
; code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "slay", main

F_I             = $01           ; -i

.bss
self:       .res        1
task:       .res        1
found:      .res        1
info:       .res        TI_SIZE

.code
main:
            jsr         tl_start
            lda         (tl_arg)
            bne         :+
            jmp         tl_badusage

:
            jsr         GETPID
            sta         self
@arg:
            stz         found
            lda         #1                                  ; Each task but the kernel's and this one
            sta         task
@task:
            lda         task
            cmp         self
            beq         @next
            LDR         r0, info
            lda         task
            jsr         TASKINFO
            bcs         @next
            lda         info + TI_STATE
            beq         @next
            ldy         #$FF                                ; Its name the one asked for?
:
            iny
            lda         info + TI_NAME,Y
            cmp         (tl_arg),Y
            bne         @next
            cmp         #0
            bne         :-
            inc         found
            ldx         #NOTE_KILL
            lda         tl_flags
            and         #F_I
            beq         :+
            ldx         #NOTE_INTERRUPT
:
            lda         task
            jsr         NOTE
@next:
            inc         task
            lda         task
            cmp         #16
            bne         @task
            lda         found
            bne         :+
            MOVR        r0, tl_arg
            lda         #E_SRCH
            jsr         tl_err
:
            jsr         tl_next
            bne         @arg
            jmp         tl_end

.rodata
tl_name:    .byte       "slay", 0
tl_flagset: .byte       "i", 0
tl_usage:   .byte       "slay [-i] name ...", 0

.include "toollib.s"
