.debuginfo
.segment "TASKS"

TASK_0_VECTOR           = $E000

TASK_BUSY_FLAG          = 1
TASK_PAUSED_FLAG        = 2

; TASK STATUS REGISTER BITS
;   0: 0 = Available, 1 = In Use

.macro SELECT_TASK      task
            lda     T_REGISTER
            and     #$F0
            ora     #(task & $0F)
            sta     T_REGISTER
.endmacro

.macro SELECT_SHARED_BANK bank
            lda     T_REGISTER
            and     #$0F
            ora     #(bank << 4)
            sta     T_REGISTER
.endmacro

; Initialize the tasks, their stacks, etc.
TASKS_INIT:
            sei                                     ; Turn off interrupts
            lda     T_REGISTER
            bne     @cleanup                        ; Only support tasks init when on task 0
            ldx     #MAX_TASK_NUMBER

@loop:
            stx     T_REGISTER                      ; Quick switch to task X
            stz     RAM_BANK_REG
            stz     ROM_BANK_REG
            stz     TASK_STATUS_REG
            stz     TASK_PARENT
            lda     #$FF
            sta     STACK_SAVE_REG
            MOV     ZP_READ_PTR, ZP_WRITE_PTR       ; Do INIT_BUFFER, without the stack
            dex
            bpl     @loop                           ; Loop back as long as X >= 0

            ; setup interrupt handler and interrupt timer

            ; Will fall through when X = $FF, leaving us in Task 0, as required

@cleanup:
            cli                                     ; Turn interrupts back on
            rts

;  Task switch
;  Task# to switch to in A
SWITCH_TO:
            sta     ZP_A_SAVE
            pla                                     ; need to change return addr from RTS style (IP - 1) to RTI style (IP)
            inc
            bne     :+                              ; page boundary?
            stx     ZP_X_SAVE
            plx
            inx
            phx
            ldx     ZP_X_SAVE
:
            pha
            php
            lda     ZP_A_SAVE

SWITCH_TO_NO_PHP:
            PUSH_AXY
            tsx
            stx     STACK_SAVE_REG

SWITCH_TO_NSS:
            sta     T_REGISTER
            ldx     STACK_SAVE_REG                  ; Restore the stack pointer
            txs                                     ; ...
            PULL_YXA
            rti

; .A.Y: Address of task entrypoint
SPAWN_TASK:
            sta     ZP_TEMP_VEC
            sty     ZP_TEMP_VEC + 1

; Find a task that is idle and start it executing at the address in ZP_TEMP_VEC && ZP_TEMP_VEC + 1
; Return task # in A and C == 1
;   OR error in A and C == 0 (if no task available)
TASK_START:
            jsr     RESERVE_TASK
            bcs     @start_task
            lda     #ERR_NO_TASKS_AVAILABLE
            rts

@start_task:
            ldy     T_REGISTER
            sta     T_REGISTER
            sty     TASK_PARENT
            sty     T_REGISTER
            ldx     ZP_TEMP_VEC + 1
            ldy     ZP_TEMP_VEC
            bne     :+
            dex                                     ; update entrypoint to rts-style addr-1
:
            dey                                     ; update LO byte
            sta     T_REGISTER                      ; do the task switch
            stx     ZP_X_SAVE                       ; new task ZP
            ldx     #$FF                            ; Reset the stack pointer
            txs
            ldx     ZP_X_SAVE
            tya
            jsr     @task_start

@task_complete:
            lda     #TASK_BUSY_FLAG
            trb     TASK_STATUS_REG
            ldx     #$FF
            stx     TASK_PARENT                     ; ...and reset the resume-to register to #$FF (invalid)
            jsr     NEXT_TASK
            jmp     SWITCH_TO_NSS

@task_start:
            phx                                     ; push the start address onto the stack
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
            ldy     T_REGISTER
            ldx     #$F                             ; Start search with Task $F

@task_busy:
            stx     T_REGISTER                      ; Quick task switch to task in .X
            bbr0    TASK_STATUS_REG, @task_found    ; Is Bit 0 (TASK_BUSY_FLAG) reset/clear?
            dex                                     ; Not found, so DEC .X
            bne     @task_busy                      ; Until .X is zero, loop
            clc                                     ; Not found
            dex                                     ; .X == $FF
            bra     @cleanup

@task_found:
            lda     #TASK_BUSY_FLAG|TASK_PAUSED_FLAG
            sta     TASK_STATUS_REG                 ; SET the Task as Busy and Paused
            sec                                     ; Found

@cleanup:
            txa                                     ; Return the task number in A (OR $FF if not found)
            sty     T_REGISTER                      ; Switch back to the original task

; Back on the original task, so restore the registers
            PULL_YX
            cli                                     ; Re-enable interrupts
            rts

; Find the next task that is paused
; Return task # to switch to in A.  C == 0, none found; C == 1, found
NEXT_TASK:
            PUSH_AXY
            lda     T_REGISTER
            and     #$0F
            tay

@test_next:
            inc
            and     #$0F                            ; masking since we could have carried
            sta     ZP_TEMP
            cpy     ZP_TEMP                         ; are we back where we started?
            beq     @not_found
            sta     T_REGISTER                      ; switch to the next task
            bbr1    TASK_STATUS_REG, @test_next     ; Is bit 1 clear (TASK_PAUSED_FLAG)? if so, try next task
            sec
            SKIPNEXT

@not_found:
            clc

@done:
            sty     T_REGISTER
            PULL_YXA
            rts

; Non-maskable interrupt handler (same as maskable interrupt handler for now)
NMI_HANDLER:
            rti

            pha
            PRINT_CHAR  #ASCII_STAR
            pla
            jsr     NEXT_TASK
            bcs     @switch
            rti

@switch:
            jmp     SWITCH_TO_NO_PHP
