.debuginfo

.segment "IRQ"

; ****************************************************************************
; IRQ dispatch (see MMU_PLAN.md, IO subsystem)
;
;   Every IRQ (and BRK) enters through a stub in the COMMON block, which saves W and switches to ROM
;   page 0, then jumps to IRQ_DISPATCH.  The dispatcher looks the IRQ up in the registration table and
;   runs each registered handler IN THE TASK THAT REGISTERED IT (via TASK_CALL), then switches back.
;
;   The registration tables live in the task system page and are replicated into every task, so the
;   dispatcher can read them from whichever task was interrupted.  Registration writes all 16 copies.
;
;   Handler convention:
;       IN:  .A = logical IRQ# (0-14), or S/W interrupt# (0-15) for S/W interrupts;  I flag set
;       OUT: C = 1 if the interrupt was claimed (stops the chain), C = 0 if not
;       May clobber .A, .X, .Y.  Must not re-enable interrupts, and must not use FAR_GATEs.

IRQ_MAX_CHAIN       = 2                                     ; Handlers per hardware IRQ
IRQ_ENTRY_SIZE      = 3                                     ; {task, handler.w}
IRQ_LOGICAL_SW      = 15                                    ; Logical IRQ# of the S/W interrupt (IRQ_NUMBER_SW)
IRQ_NO_TASK         = $FF                                   ; Empty table entry

IRQ_SYS_BASE        = MMU_SYS_PAGE * $100                   ; Task system page ($7D00)
IRQ_TABLE           = IRQ_SYS_BASE                          ; 16 IRQs x IRQ_MAX_CHAIN x {task, handler.w}
IRQ_SWI_OFFSET      = 16 * IRQ_MAX_CHAIN * IRQ_ENTRY_SIZE   ; 16 S/W interrupts x {task, handler.w} follow
IRQ_TABLE_SIZE      = IRQ_SWI_OFFSET + (16 * IRQ_ENTRY_SIZE)
IRQ_SPURIOUS        = IRQ_TABLE + IRQ_TABLE_SIZE            ; 16 counters, by logical IRQ# ($7D90-$7D9F in the interrupted
                                                            ; task): IRQs not claimed by their own handlers

.assert     IRQ_TABLE_SIZE <= 256, error, "IRQ table must be indexable by .X"

; Set up the IRQ tables in every task's system page, and point all 16 vectors at the IRQ stubs.
; Must be called before interrupts are enabled.
IRQ_INIT:
            php                                             ; Save caller's I flag
            sei
            PUSH_AXY
            ldx         T_REGISTER                          ; Task 0 holds the home task # during the loop
            stz         T_REGISTER
            stx         ZP_IRQ_HOME
            ldx         #MAX_TASK_NUMBER

@task_loop:
            stx         T_REGISTER                          ; Quick switch to task X (no stack use!)
            ldy         #IRQ_TABLE_SIZE
            lda         #IRQ_NO_TASK

@clear_table:
            dey
            sta         IRQ_TABLE,Y
            bne         @clear_table
            ldy         #16
            lda         #0

@clear_spurious:
            dey
            sta         IRQ_SPURIOUS,Y
            bne         @clear_spurious
            dex
            bpl         @task_loop                          ; Ends in task 0
            ldx         ZP_IRQ_HOME
            stx         T_REGISTER                          ; Back to the home task

            LOAD_ADDR   IRQ_STUB_0, ZP_TEMP_VEC
            ldx         #0                                  ; Logical IRQ#

@vector_loop:
            phx
            txa
            eor         #7                                  ; IRQ_NUMBER(): logical -> vector index
            tax
            lda         ZP_TEMP_VEC
            ldy         ZP_TEMP_VEC + 1
            jsr         IRQ_SET_VECTOR
            plx
            lda         ZP_TEMP_VEC                         ; Next stub (6 bytes each)
            clc
            adc         #6
            sta         ZP_TEMP_VEC
            bcc         :+
            inc         ZP_TEMP_VEC + 1
:
            inx
            cpx         #16
            bne         @vector_loop
            PULL_YXA
            plp                                             ; Restore caller's I flag
            clc
            rts

; X: IRQ# (vector index, i.e. IRQ_NUMBER(n)), .A.Y: Vector Addr
; Preserves .A, .X, .Y, V and the caller's I flag
IRQ_SET_VECTOR:
            php                                             ; Save caller's I flag
            sei
            phx
            pha
            lda         V_REGISTER                          ; Save V (shared pseudo-register)
            stx         V_REGISTER                          ; Select the vector for IRQ# .X
            tax                                             ; .X = prior V
            pla
            sta         $FFFE
            sty         $FFFF
            stx         V_REGISTER                          ; Restore prior V
            plx
            plp                                             ; Restore caller's I flag
            rts

; Entered from IRQ_ENTRY (COMMON block) on ROM page 0.
; .A = logical IRQ#, .X = interrupted ROM page.  Stack: .X, .A, then the interrupt frame.
IRQ_DISPATCH:
            phx                                             ; Interrupted ROM page, for IRQ_EXIT
            phy
            cld
            ldx         ZP_TC_VEC                           ; The interrupted code may be setting up a TASK_CALL
            phx
            ldx         ZP_TC_VEC + 1
            phx
            ldx         ZP_TC_TASK
            phx
            cmp         #IRQ_LOGICAL_SW
            beq         @swi
            sta         ZP_IRQ_NUM
            asl                                             ; x6 = IRQ_MAX_CHAIN * IRQ_ENTRY_SIZE
            adc         ZP_IRQ_NUM
            asl
            tax
            jsr         IRQ_CALL_ENTRY
            bcs         @done
            inx
            inx
            inx
            jsr         IRQ_CALL_ENTRY
            bcs         @done
            ldx         ZP_IRQ_NUM                          ; Nobody on its own chain claimed it
            inc         IRQ_SPURIOUS,X
            ldx         #0                                  ; Offer it to every registered hardware handler: each
                                                            ; checks its own device, so an IRQ that arrives on an
@poll_all:                                                  ; unexpected vector still gets cleared (no IRQ storm)
            jsr         IRQ_CALL_ENTRY
            bcs         @done
            inx
            inx
            inx
            cpx         #IRQ_SWI_OFFSET
            bne         @poll_all
            bra         @done

@swi:
            lda         V_REGISTER                          ; S/W interrupt# is in V[4..7]
            lsr
            lsr
            lsr
            lsr
            sta         ZP_IRQ_NUM
            asl                                             ; x3 = IRQ_ENTRY_SIZE
            adc         ZP_IRQ_NUM
            adc         #IRQ_SWI_OFFSET
            tax
            jsr         IRQ_CALL_ENTRY

@done:
            pla
            sta         ZP_TC_TASK
            pla
            sta         ZP_TC_VEC + 1
            pla
            sta         ZP_TC_VEC
            ply
            pla                                             ; Interrupted ROM page
            jmp         IRQ_EXIT

; Run the handler in table entry .X (if any), in its task.
; IN: .X = table offset.  OUT: C = 1 if claimed.  Preserves .X
IRQ_CALL_ENTRY:
            stx         ZP_IRQ_TMP
            lda         IRQ_TABLE,X
            cmp         #IRQ_NO_TASK
            beq         @none
            sta         ZP_TC_TASK
            lda         IRQ_TABLE + 1,X
            sta         ZP_TC_VEC
            lda         IRQ_TABLE + 2,X
            sta         ZP_TC_VEC + 1
            lda         ZP_IRQ_NUM
            jsr         TASK_CALL
            ldx         ZP_IRQ_TMP
            rts

@none:
            clc
            rts

; ****************************************************************************
; Registration.  A handler always runs in the task that registered it (the current task), so drivers
; register from their init routine, which DRV_START runs in the driver's task.

; Register a handler for a hardware IRQ.  Registering the same handler twice from the same task is OK.
; IN: .X = IRQ# (IRQ_NUMBER(n), as used for V), .A.Y = handler
; OUT (success): C = 0
; OUT (failure): .A = ERR_IRQ_CHAIN_FULL, C = 1
; Preserves .X, .Y
IRQ_REGISTER:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            jsr         IRQ_HW_SETUP
            jsr         IRQ_REG_AT
            PULL_YX
            jmp         MM_RETURN

; Unregister a handler registered from the current task.
; IN: .X = IRQ# (IRQ_NUMBER(n)), .A.Y = handler
; OUT (success): C = 0
; OUT (failure): .A = ERR_IRQ_NOT_FOUND, C = 1
; Preserves .X, .Y
IRQ_UNREGISTER:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            jsr         IRQ_HW_SETUP
            jsr         IRQ_UNREG_AT
            PULL_YX
            jmp         MM_RETURN

; Register a handler for a S/W interrupt (SW_INT).  One handler per S/W interrupt#.
; IN: .X = S/W interrupt# ($0-$F), .A.Y = handler
; OUT (success): C = 0
; OUT (failure): .A = ERR_IRQ_CHAIN_FULL, C = 1
; Preserves .X, .Y
SWI_REGISTER:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            jsr         IRQ_SWI_SETUP
            jsr         IRQ_REG_AT
            PULL_YX
            jmp         MM_RETURN

; Unregister a S/W interrupt handler registered from the current task.
; IN: .X = S/W interrupt# ($0-$F), .A.Y = handler
; OUT (success): C = 0
; OUT (failure): .A = ERR_IRQ_NOT_FOUND, C = 1
; Preserves .X, .Y
SWI_UNREGISTER:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            jsr         IRQ_SWI_SETUP
            jsr         IRQ_UNREG_AT
            PULL_YX
            jmp         MM_RETURN

; IN: .X = IRQ_NUMBER(n), .A.Y = handler.  OUT: .X = first table offset, ZP_IRQ_CNT, ZP_IRQ_H set
IRQ_HW_SETUP:
            sta         ZP_IRQ_H
            sty         ZP_IRQ_H + 1
            txa
            eor         #7                                  ; Vector index -> logical IRQ#
            and         #$0F
            sta         ZP_IRQ_TMP
            asl                                             ; x6 = IRQ_MAX_CHAIN * IRQ_ENTRY_SIZE
            adc         ZP_IRQ_TMP
            asl
            tax
            lda         #IRQ_MAX_CHAIN
            sta         ZP_IRQ_CNT
            rts

; IN: .X = S/W interrupt#, .A.Y = handler.  OUT: .X = table offset, ZP_IRQ_CNT, ZP_IRQ_H set
IRQ_SWI_SETUP:
            sta         ZP_IRQ_H
            sty         ZP_IRQ_H + 1
            txa
            and         #$0F
            sta         ZP_IRQ_TMP
            asl                                             ; x3 = IRQ_ENTRY_SIZE
            adc         ZP_IRQ_TMP
            adc         #IRQ_SWI_OFFSET
            tax
            lda         #1
            sta         ZP_IRQ_CNT
            rts

; Add (current task, ZP_IRQ_H) to the ZP_IRQ_CNT entries starting at offset .X, unless already there.
IRQ_REG_AT:
            stx         ZP_IRQ_TMP
            ldy         ZP_IRQ_CNT

@find_existing:
            jsr         IRQ_MATCH
            beq         @ok                                 ; Already registered
            inx
            inx
            inx
            dey
            bne         @find_existing
            ldx         ZP_IRQ_TMP
            ldy         ZP_IRQ_CNT

@find_free:
            lda         IRQ_TABLE,X
            cmp         #IRQ_NO_TASK
            beq         @store
            inx
            inx
            inx
            dey
            bne         @find_free
            lda         #ERR_IRQ_CHAIN_FULL
            sec
            rts

@store:
            txa
            tay                                             ; .Y = entry offset
            iny
            lda         ZP_IRQ_H
            jsr         IRQ_REPLICATE
            iny
            lda         ZP_IRQ_H + 1
            jsr         IRQ_REPLICATE
            dey
            dey
            lda         T_REGISTER                          ; Task byte last: it makes the entry live
            jsr         IRQ_REPLICATE

@ok:
            clc
            rts

; Remove (current task, ZP_IRQ_H) from the ZP_IRQ_CNT entries starting at offset .X.
IRQ_UNREG_AT:
            ldy         ZP_IRQ_CNT

@find:
            jsr         IRQ_MATCH
            beq         @remove
            inx
            inx
            inx
            dey
            bne         @find
            lda         #ERR_IRQ_NOT_FOUND
            sec
            rts

@remove:
            txa
            tay                                             ; .Y = entry offset
            lda         #IRQ_NO_TASK
            jsr         IRQ_REPLICATE
            clc
            rts

; Z = 1 if table entry .X is (current task, ZP_IRQ_H).  Preserves .X, .Y
IRQ_MATCH:
            lda         IRQ_TABLE,X
            cmp         T_REGISTER
            bne         @done
            lda         IRQ_TABLE + 1,X
            cmp         ZP_IRQ_H
            bne         @done
            lda         IRQ_TABLE + 2,X
            cmp         ZP_IRQ_H + 1

@done:
            rts

; Write .A to IRQ_TABLE + .Y in every task's system page.  I flag must be set.
; Preserves .A, .Y
IRQ_REPLICATE:
            ldx         T_REGISTER                          ; Task 0 holds the home task # during the loop
            stz         T_REGISTER
            stx         ZP_IRQ_HOME
            ldx         #MAX_TASK_NUMBER

@loop:
            stx         T_REGISTER                          ; Quick switch to task X (no stack use!)
            sta         IRQ_TABLE,Y
            dex
            bpl         @loop                               ; Ends in task 0
            ldx         ZP_IRQ_HOME
            stx         T_REGISTER                          ; Back to the home task
            rts
