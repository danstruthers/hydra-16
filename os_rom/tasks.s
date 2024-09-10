.debuginfo
.segment "TASKS"

TASK_0_VECTOR           = $E000
RAM_BANK_REG            = $0
ROM_BANK_REG            = $1
TASK_STATUS_REG         = $2
TASK_PARENT             = $3
STACK_SAVE_REG          = $0100
TASK_VECTOR_BASE        = $7E00
TASK_NUM_PORT           = IO_PORT_BYTE IO_PORT_F, 0

TASK_BUSY_FLAG          = $01
TASK_PAUSED_FLAG        = $02

; TASK STATUS REGISTER BITS
;   0: 0 = Available, 1 = In Use

; Initialize the tasks, their stacks, etc.
TASKS_INIT:
            sei                                     ; Turn off interrupts
            lda     TASK_NUM_PORT
            bne     @cleanup                        ; Only support task init when on task 0 
            ldx     #MAX_TASK_NUMBER

@loop:
            lda     #0
            stx     TASK_NUM_PORT                   ; Quick switch to task X
            sta     RAM_BANK_REG
            sta     ROM_BANK_REG
            sta     TASK_STATUS_REG
            sta     TASK_PARENT
            lda     #$FF
            sta     STACK_SAVE_REG
            MOV     ZP_READ_PTR, ZP_WRITE_PTR             ; Do INIT_BUFFER, without the stack
            dex
            bpl     @loop                           ; Loop back as long as X >= 0
            ; Will fall through when X = $FF, leaving us in Task 0, as required

@cleanup:
            cli                                     ; Turn interrupts back on
            rts

;  Task switch
;  Task# to switch to in A
SWITCH_TO:
            php

SWITCH_TO_NO_PHP:
            PUSH_AXY
            txs
            stx     STACK_SAVE_REG

SWITCH_TO_NSS:
            sta     TASK_NUM_PORT
            ldx     STACK_SAVE_REG                  ; Restore the stack pointer
            txs                                     ; ...
            PULL_YXA
            rti

; Find a task that is idle and start it executing at the address in ZP_TEMP_VEC_L && Z_TEMP_VEC_H
; Return task # in A and C == 1
;   OR error in A and C == 0 (if no task available)
TASK_START:
            jsr     RESERVE_TASK
            bcs     @start_task
            lda     #ERR_NO_TASKS_AVAILABLE
            rts

@start_task:
            tay
            lda     TASK_NUM_PORT                   ; save current task as new task's parent
            sty     TASK_NUM_PORT
            sta     TASK_PARENT
            sta     TASK_NUM_PORT                   ; get the new task start addr in A/X
            lda     ZP_TEMP_VEC_L
            ldx     ZP_TEMP_VEC_H
            sty     TASK_NUM_PORT                   ; do the task switch
            stx     ZP_X_SAVE                       ; new task ZP
            ldx     #$FF                            ; Reset the stack pointer
            txs
            ldx     ZP_X_SAVE
            jsr     @task_start

@task_complete:
.ifpc02
            lda     #TASK_BUSY_FLAG
            trb     TASK_STATUS_REG
.else
            lda     #TASK_BUSY_FLAG^$FF
            and     TASK_STATUS_REG                 ; Set the task to no longer running
            sta     TASK_STATUS_REG
.endif
            ldx     #$FF
            stx     TASK_PARENT                     ; ...and reset the resume-to register to #$FF (invalid)
            jsr     NEXT_TASK
            jmp     SWITCH_TO_NSS

@task_start:
            PUSH_X                                  ; push the start address onto the stack
            pha                                     ; ...
            rts                                     ; start executing


; Find an available task
; Modifies: A, CNZ Flags
; Returns C = 1 AND A = TaskNumber (when found)
; Returns C = 0 AND A = $FF        (when not found)
RESERVE_TASK:
            sei                                     ; Disable interrupts
            PUSH_XY

; !! NO STACK MANIPULATIONS UNTIL SWITCHING BACK TO ORIGINAL TASK !!
            lda     #0
            ldy     TASK_NUM_PORT
            lda     #TASK_BUSY_FLAG
            ldx     #$F                             ; Start search with Task $F

@task_busy:
            stx     TASK_NUM_PORT                   ; Quick task switch to task X
            bit     TASK_STATUS_REG                 ; Is Bit 1 set?
            bne     @task_found
            dex                                     ; Not found, so DEC X
            bne     @task_busy                      ; Until X is zero, loop
            clc                                     ; Not found
            dex                                     ; X == $FF
            bcc     @cleanup

@task_found:
            lda     #TASK_BUSY_FLAG|TASK_PAUSED_FLAG
            sta     TASK_STATUS_REG                 ; SET the Task as Busy and Paused
            sec                                     ; Found

@cleanup:
            txa                                     ; Return the task number in A (OR $FF if not found)
            sty     TASK_NUM_PORT                   ; Switch back to the original task

; Back on the original task, so restore the registers
            PULL_YX
            cli                                     ; Re-enable interrupts
            rts

; Find the next task that is paused
; Return task # to switch to in A.  C == 0, none found; C == 1, found
NEXT_TASK:
            PUSH_AX
            lda     TASK_NUM_PORT
            sta     ZP_X_SAVE
            and     #$0F                            ; mask off the shared memory "bank of banks"
            sta     ZP_A_SAVE
            tax

@test_next:
            inx
            txa
            and     #$0F                            ; masking again since we could have carried
            cmp     ZP_A_SAVE                       ; are we back where we started?
            beq     @not_found
            sta     TASK_NUM_PORT                   ; switch to the next task
            lda     #TASK_PAUSED_FLAG
            and     TASK_STATUS_REG                 ; is this task paused?
            beq     @test_next                      ; no? try the next one
            sec
            bcs     @done

@not_found:
            clc

@done:
            ldx     ZP_X_SAVE                       ; switch back to the original task
            stx     TASK_NUM_PORT
            PULL_XA
            rts

; Non-maskable interrupt handler (same as maskable interrupt handler for now)
NMI_HANDLER:
            pha
            lda     #ASCII_STAR
            jsr     WRITECHAR
            pla
            jsr     NEXT_TASK
            bcs     @switch
            rti

@switch:
            jmp SWITCH_TO_NO_PHP
