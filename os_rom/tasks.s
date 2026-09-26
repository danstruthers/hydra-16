.debuginfo
.segment "TASKS"

TASK_0_VECTOR           = $E000

TASK_BUSY_FLAG          = 1
TASK_PAUSED_FLAG        = 2
TASK_RESIDENT_FLAG      = 8

; TASK STATUS REGISTER BITS
;   0: 0 = Available, 1 = In Use
;   1: 0 = Active, 1 = Paused
;   2: 0 = Not Waiting, 1 = Awaiting I/O
;   3: 0 = , 1 = Resident (driver task: runs only from IRQs / TASK_CALLs, never scheduled)
;   4: 0 = , 1 =
;   5: 0 = , 1 =
;   6: 0 = , 1 =
;   7: 0 = , 1 =

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
            stz     ZP_D_PAGE                       ; Disassembler reads the BIOS page by default
            lda     #$FF
            sta     STACK_SAVE_REG
            dex
            bpl     @loop                           ; Loop back as long as X >= 0
            lda     #TASK_BUSY_FLAG
            sta     TASK_STATUS_REG                 ; Mark task zero as "busy"

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
            sei                                     ; No IRQs between the task and stack switch (RTI restores I)
            PUSH_AXY
            tsx
            stx     STACK_SAVE_REG

SWITCH_TO_NSS:
            sei
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
            cmp     T_REGISTER
            bne     :+                              ; task is current task, so just bail out
            rts

:
            ldy     T_REGISTER
            sta     T_REGISTER
            sty     TASK_PARENT
            sty     T_REGISTER
            ldx     ZP_TEMP_VEC + 1
            ldy     ZP_TEMP_VEC
            bne     :+                              ; skip HOB of addr if LOB <> 0
            dex                                     ; update entrypoint to rts-style addr-1

:
            dey                                     ; update LOB
            smb1    TASK_STATUS_REG                 ; mark parent task state as PAUSED
            sta     T_REGISTER                      ; do the task switch
            stx     ZP_X_SAVE                       ; new task ZP
            ldx     #$FF                            ; Reset the stack pointer
            txs
            ldx     ZP_X_SAVE
            tya
            jsr     @task_start

@task_complete:
            stz     TASK_STATUS_REG
            lda     TASK_PARENT
            ldx     #$FF
            stx     TASK_PARENT                     ; ...and reset the resume-to register to #$FF (invalid)
            jmp     SWITCH_TO_NSS

@task_start:
            rmb1    TASK_STATUS_REG                 ; remove the PAUSED flag
            phx                                     ; push the start address onto the stack
            pha                                     ; ...
            rts                                     ; start executing


; Find an available task
; Modifies: A, CNZ Flags
; Returns C = 1 AND A = TaskNumber (when found)
; Returns C = 0 AND A = $FF        (when not found)
RESERVE_TASK:
            php                                     ; Save caller's I flag
            sei                                     ; Disable interrupts
            PUSH_XY

; !! NO STACK MANIPULATIONS UNTIL SWITCHING BACK TO ORIGINAL TASK !!
            ldy     T_REGISTER
            ldx     #$F                             ; Start search with Task $F

@task_busy:
            stx     T_REGISTER                      ; Quick task switch to task in .X
            bbr0    TASK_STATUS_REG, @task_found    ; Is Bit 0 (TASK_BUSY_FLAG) reset/clear?
            dex                                     ; Not found, so check next
            bne     @task_busy                      ; Until we reach the system task (0), loop
            clc                                     ; Not found
            dex                                     ; .X == $FF
            bra     @cleanup

@task_found:
            smb0    TASK_STATUS_REG
            smb1    TASK_STATUS_REG
            sec                                     ; Found

@cleanup:
            txa                                     ; Return the task number in A (OR $FF if not found)
            sty     T_REGISTER                      ; Switch back to the original task

; Back on the original task, so restore the registers
            PULL_YX
            jmp     MM_RETURN                       ; Restore caller's I flag, keep C

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

; ****************************************************************************
; Run a routine in another task's context: its ZP, stack (below its saved SP), RAM bank and MMU area.
; Used by the IRQ dispatcher and DRV_START, and by gates into driver tasks.
;
; IN:  ZP_TC_VEC = routine, ZP_TC_TASK = task to run it in, .A/.X/.Y/C = routine's inputs
; OUT: .A/.X/.Y/flags as returned by the routine
; The routine runs with the caller's I flag.  The target task must not be running, or it's the
; current task (then this is a plain call).
; The calling task # is kept on the target task's stack, so IRQs during the routine are safe.
.macro _M_TC_COPY_TO    zp                  ; ZP byte: calling task (.X) -> target task (.Y); ends in calling task
            lda     zp
            sty     T_REGISTER
            sta     zp
            stx     T_REGISTER
.endmacro

.macro _M_TC_COPY_BACK  zp                  ; ZP byte: target task (.Y) -> calling task (.X); ends in target task
            lda     zp
            stx     T_REGISTER
            sta     zp
            sty     T_REGISTER
.endmacro

TASK_CALL:
            php
            sei
            sta     ZP_TC_A
            pla
            sta     ZP_TC_P                         ; Caller's flags (C in, I state)
            stx     ZP_TC_X
            sty     ZP_TC_Y
            lda     ZP_TC_TASK
            cmp     T_REGISTER
            bne     @switch
            lda     ZP_TC_P                         ; Same task: restore flags and tail-call
            pha
            lda     ZP_TC_A
            plp
            jmp     (ZP_TC_VEC)

@switch:
            lda     STACK_SAVE_REG                  ; Preserve the calling task's saved SP
            pha
            tsx
            stx     STACK_SAVE_REG
            lda     T_REGISTER
            sta     ZP_TC_FROM
            tax                                     ; .X = calling task
            ldy     ZP_TC_TASK                      ; .Y = target task
            _M_TC_COPY_TO   ZP_TC_VEC
            _M_TC_COPY_TO   ZP_TC_VEC + 1
            _M_TC_COPY_TO   ZP_TC_A
            _M_TC_COPY_TO   ZP_TC_X
            _M_TC_COPY_TO   ZP_TC_Y
            _M_TC_COPY_TO   ZP_TC_P
            _M_TC_COPY_TO   ZP_TC_FROM

; !! NO STACK MANIPULATIONS UNTIL THE TARGET TASK'S STACK IS SELECTED !!
            sty     T_REGISTER                      ; Switch to the target task
            ldx     STACK_SAVE_REG                  ; ...and its stack
            txs
            lda     ZP_TC_FROM
            pha                                     ; Keep the calling task # on the target's stack
            lda     ZP_TC_P
            pha
            ldx     ZP_TC_X
            ldy     ZP_TC_Y
            lda     ZP_TC_A
            plp                                     ; Caller's flags
            jsr     @call
            php
            sei
            sta     ZP_TC_A
            stx     ZP_TC_X
            sty     ZP_TC_Y
            pla
            sta     ZP_TC_P                         ; Routine's result flags
            pla
            tax                                     ; .X = calling task
            ldy     T_REGISTER                      ; .Y = target task
            _M_TC_COPY_BACK ZP_TC_A
            _M_TC_COPY_BACK ZP_TC_X
            _M_TC_COPY_BACK ZP_TC_Y
            _M_TC_COPY_BACK ZP_TC_P

; !! NO STACK MANIPULATIONS UNTIL THE CALLING TASK'S STACK IS SELECTED !!
            stx     T_REGISTER                      ; Back to the calling task
            ldx     STACK_SAVE_REG                  ; ...and its stack
            txs
            pla
            sta     STACK_SAVE_REG                  ; Restore the calling task's saved SP
            ldx     ZP_TC_X
            ldy     ZP_TC_Y
            lda     ZP_TC_P
            pha
            lda     ZP_TC_A
            plp                                     ; Routine's result flags
            rts

@call:
            jmp     (ZP_TC_VEC)

; ****************************************************************************
; Make a free task ready to run from an entry point: marks it busy and builds the frame SWITCH_TO
; resumes from on its stack (A/X/Y = 0, IRQs enabled).  Start it with SWITCH_TO.
; The entry routine must never return (there is nothing to return to).
; IN: .A.Y = entry point, .X = task#
; OUT (success): C = 0
; OUT (failure): .A = ERR_BAD_TASK or ERR_TASK_BUSY, C = 1
; Modifies: .A, .Y
TASK_PREPARE:
            php                                     ; Save caller's I flag
            sei
            cpx     #MAX_TASK_NUMBER + 1
            bcs     @bad_task
            cpx     T_REGISTER
            beq     @bad_task
            sta     ZP_TEMP_VEC
            sty     ZP_TEMP_VEC + 1
            ldy     T_REGISTER                      ; .Y = calling task, .X = new task

; !! NO STACK MANIPULATIONS WHILE IN THE NEW TASK !!
            stx     T_REGISTER                      ; Quick switch to the new task
            lda     TASK_STATUS_REG
            sty     T_REGISTER
            bne     @busy
            lda     ZP_TEMP_VEC + 1
            stx     T_REGISTER
            sta     $01FF                           ; RTI frame: PCH, PCL, P
            sty     T_REGISTER
            lda     ZP_TEMP_VEC
            stx     T_REGISTER
            sta     $01FE
            lda     #0
            sta     $01FD                           ; P: IRQs enabled, decimal off
            sta     $01FC                           ; PULL_YXA frame: A, X, Y
            sta     $01FB
            sta     $01FA
            lda     #$F9
            sta     STACK_SAVE_REG
            lda     #TASK_BUSY_FLAG
            sta     TASK_STATUS_REG
            sty     T_REGISTER                      ; Back to the calling task
            clc
            jmp     MM_RETURN

@busy:
            lda     #ERR_TASK_BUSY
            sec
            jmp     MM_RETURN

@bad_task:
            lda     #ERR_BAD_TASK
            sec
            jmp     MM_RETURN

; ****************************************************************************
; Drivers

.struct     DriverInfo
            init        .word                       ; Runs in the driver's task.  OUT: C = 0 OK, or C = 1 and .A = error
            stop        .word                       ; (future) Runs in the driver's task before the task is reset
            name        .word                       ; HString
.endstruct

; Start a driver in a (free) task, as a Resident task: its init runs in that task, so the driver's
; state lives in that task's ZP/RAM, and IRQ handlers it registers run in that task.
; IN: .A.Y = DriverInfo, .X = task#
; OUT (success): .A = task#, C = 0
; OUT (failure): .A = ERROR, C = 1 (the task is left free)
; Modifies: .A, .X, .Y
DRV_START:
            php                                     ; Save caller's I flag
            sei
            sta     ZP_DRV_PTR
            sty     ZP_DRV_PTR + 1
            cpx     #MAX_TASK_NUMBER + 1
            bcs     @bad_task
            cpx     T_REGISTER
            beq     @bad_task
            stx     ZP_TC_TASK
            ldy     T_REGISTER
            stx     T_REGISTER                      ; Quick switch to the driver task (no stack use!)
            lda     TASK_STATUS_REG
            bne     @busy
            lda     #TASK_BUSY_FLAG | TASK_RESIDENT_FLAG
            sta     TASK_STATUS_REG
            sty     T_REGISTER                      ; Back to the calling task
            ldy     #DriverInfo::init
            lda     (ZP_DRV_PTR),Y
            sta     ZP_TC_VEC
            iny
            lda     (ZP_DRV_PTR),Y
            sta     ZP_TC_VEC + 1
            lda     ZP_TC_TASK                      ; init gets its task# in .A
            jsr     TASK_CALL
            bcs     @init_failed
            lda     ZP_TC_TASK
            clc
            jmp     MM_RETURN

@init_failed:
            ldx     ZP_TC_TASK
            ldy     T_REGISTER
            stx     T_REGISTER                      ; Quick switch to the driver task (no stack use!)
            stz     TASK_STATUS_REG                 ; Leave it free
            sty     T_REGISTER
            sec                                     ; .A = error from init
            jmp     MM_RETURN

@busy:
            sty     T_REGISTER                      ; Back to the calling task
            lda     #ERR_TASK_BUSY
            sec
            jmp     MM_RETURN

@bad_task:
            lda     #ERR_BAD_TASK
            sec
            jmp     MM_RETURN

; Non-maskable interrupt handler, called from NMI_ENTRY (COMMON block) on ROM page 0
NMI_HANDLER:
            rts

            pha
            PRINT_CHAR  #ASCII_STAR
            pla
            jsr     NEXT_TASK
            bcs     @switch
            rti

@switch:
            jmp     SWITCH_TO_NO_PHP
