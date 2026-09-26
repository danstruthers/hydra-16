.debuginfo

.segment "MSG"

; ****************************************************************************
; Message rings (see MMU_PLAN.md, Messaging)
;
;   One 256-byte ring for each (receiver, sender) task pair, in shared RAM, so each ring has exactly one
;   writer and one reader.  The sender is always the current task, and the receiver reads its own rings.
;
;   Shared bank ID $00 (U = 0, bank $F0):  ring pointers, indexed by receiver * 16 + sender
;       $8000-$80FF  MSG_HEADS  write pointers (advanced by the sender)
;       $8100-$81FF  MSG_TAILS  read pointers (advanced by the receiver)
;   Shared bank IDs $01-$08 (U = 0, banks $F1-$F8):  the rings
;       bank ID = 1 + (receiver >> 1), address = $8000 + (receiver & 1) * $1000 + sender * $100
;
;   This is a byte-stream layer; framed messages (type, length, payload) can be built on top of it.
;   All routines run with IRQs off and save/restore RAM_BANK_REG and U, so they're safe to call from
;   IRQ handlers (e.g. a driver delivering input to another task).

MSG_SHARED_U        = 0                                     ; U (shared macro-page) for all message data
MSG_PTR_BANK        = $F0                                   ; Shared bank ID $00
MSG_RING_BANK       = $F1                                   ; Shared bank ID $01: rings for receivers 0 and 1
MSG_HEADS           = $8000
MSG_TAILS           = $8100
MSG_RING_BASE       = $8000

; Save RAM_BANK_REG and U on the stack and select the ring pointer bank.  Uses .Y
.macro _M_MSG_ENTER
            ldy         RAM_BANK_REG
            phy
            ldy         U_REGISTER
            phy
            ldy         #MSG_SHARED_U
            sty         U_REGISTER
            ldy         #MSG_PTR_BANK
            sty         RAM_BANK_REG
.endmacro

; Restore U and RAM_BANK_REG.  Uses .Y; preserves .A and C
.macro _M_MSG_LEAVE
            ply
            sty         U_REGISTER
            ply
            sty         RAM_BANK_REG
.endmacro

; Clear all ring pointers.  Called by the system task at boot.
MSG_INIT:
            php                                             ; Save caller's I flag
            sei
            PUSH_AXY
            _M_MSG_ENTER
            ldy         #0
            tya

@loop:
            sta         MSG_HEADS,Y
            sta         MSG_TAILS,Y
            iny
            bne         @loop
            _M_MSG_LEAVE
            PULL_YXA
            plp                                             ; Restore caller's I flag
            rts

; Send a byte from the current task to another task.
; IN: .A = byte, .X = receiving task
; OUT (success): C = 0
; OUT (failure): .A = ERR_MSG_FULL or ERR_BAD_TASK, C = 1
; Preserves .X, .Y
MSG_SEND_BYTE:
            php                                             ; Save caller's I flag
            sei
            phy
            phx
            sta         ZP_MSG_BYTE
            cpx         #MAX_TASK_NUMBER + 1
            bcs         @bad_task
            txa
            asl                                             ; Receiver * 16
            asl
            asl
            asl
            sta         ZP_MSG_IDX
            lda         T_REGISTER                          ; + sender
            and         #$0F
            ora         ZP_MSG_IDX
            sta         ZP_MSG_IDX
            _M_MSG_ENTER
            ldx         ZP_MSG_IDX
            lda         MSG_HEADS,X
            sta         ZP_MSG_PTR                          ; Ring offset = head
            inc
            cmp         MSG_TAILS,X
            beq         @full                               ; head + 1 == tail: the ring is full
            jsr         MSG_RING_SELECT
            lda         ZP_MSG_BYTE
            sta         (ZP_MSG_PTR)
            lda         #MSG_PTR_BANK
            sta         RAM_BANK_REG
            inc         MSG_HEADS,X
            clc
            bra         @leave

@full:
            lda         #ERR_MSG_FULL
            sec

@leave:
            _M_MSG_LEAVE

@done:
            plx
            ply
            jmp         MM_RETURN

@bad_task:
            lda         #ERR_BAD_TASK
            sec
            bra         @done

; Receive a byte sent to the current task.
; IN: .X = sending task, or $FF for any sender (checked from task 0 up)
; OUT (success): .A = byte, .X = sending task, C = 0
; OUT (failure): .A = ERR_MSG_EMPTY or ERR_BAD_TASK, C = 1, .X unchanged
; Preserves .Y
MSG_RECV_BYTE:
            php                                             ; Save caller's I flag
            sei
            phy
            phx
            lda         T_REGISTER                          ; Receiver * 16
            and         #$0F
            asl
            asl
            asl
            asl
            sta         ZP_MSG_IDX
            _M_MSG_ENTER
            cpx         #$FF
            beq         @any
            cpx         #MAX_TASK_NUMBER + 1
            bcs         @bad_task
            txa                                             ; + sender
            ora         ZP_MSG_IDX
            tax
            lda         MSG_HEADS,X
            cmp         MSG_TAILS,X
            beq         @empty
            bra         @read

@any:
            ldx         ZP_MSG_IDX                          ; Sender 0
            ldy         #MAX_TASK_NUMBER + 1

@scan:
            lda         MSG_HEADS,X
            cmp         MSG_TAILS,X
            bne         @read
            inx
            dey
            bne         @scan

@empty:
            lda         #ERR_MSG_EMPTY

@fail:
            sec
            _M_MSG_LEAVE
            plx
            ply
            jmp         MM_RETURN

@bad_task:
            lda         #ERR_BAD_TASK
            bra         @fail

@read:                                                      ; .X = ring index
            stx         ZP_MSG_IDX
            lda         MSG_TAILS,X
            sta         ZP_MSG_PTR                          ; Ring offset = tail
            jsr         MSG_RING_SELECT
            lda         (ZP_MSG_PTR)
            sta         ZP_MSG_BYTE
            lda         #MSG_PTR_BANK
            sta         RAM_BANK_REG
            inc         MSG_TAILS,X
            _M_MSG_LEAVE
            pla                                             ; Drop the caller's .X
            txa
            and         #$0F
            tax                                             ; .X = sending task
            lda         ZP_MSG_BYTE
            ply
            clc
            jmp         MM_RETURN

; Number of bytes waiting from a sender to the current task.
; IN: .X = sending task
; OUT (success): .A = bytes waiting, C = 0
; OUT (failure): .A = ERR_BAD_TASK, C = 1
; Preserves .X, .Y
MSG_PEEK:
            php                                             ; Save caller's I flag
            sei
            phy
            phx
            cpx         #MAX_TASK_NUMBER + 1
            bcs         @bad_task
            lda         T_REGISTER                          ; Receiver * 16 + sender
            and         #$0F
            asl
            asl
            asl
            asl
            sta         ZP_MSG_IDX
            txa
            ora         ZP_MSG_IDX
            tax
            _M_MSG_ENTER
            lda         MSG_HEADS,X
            sec
            sbc         MSG_TAILS,X
            _M_MSG_LEAVE
            clc

@done:
            plx
            ply
            jmp         MM_RETURN

@bad_task:
            lda         #ERR_BAD_TASK
            sec
            bra         @done

; Empty every ring the task sends or receives on (for task reset).
; IN: .A = task
; Preserves .A, .X, .Y
MSG_RESET_TASK:
            php                                             ; Save caller's I flag
            sei
            PUSH_AXY
            and         #$0F
            sta         ZP_MSG_BYTE
            _M_MSG_ENTER
            asl                                             ; Rings where the task receives: task * 16 + (0..15)
            asl
            asl
            asl
            tax
            ldy         #MAX_TASK_NUMBER + 1

@receives:
            lda         MSG_HEADS,X
            sta         MSG_TAILS,X
            inx
            dey
            bne         @receives
            ldx         ZP_MSG_BYTE                         ; Rings where the task sends: (0..15) * 16 + task
            ldy         #MAX_TASK_NUMBER + 1

@sends:
            lda         MSG_HEADS,X
            sta         MSG_TAILS,X
            txa
            clc
            adc         #16
            tax
            dey
            bne         @sends
            _M_MSG_LEAVE
            PULL_YXA
            plp                                             ; Restore caller's I flag
            rts

; Point ZP_MSG_PTR + 1 and RAM_BANK_REG at ring ZP_MSG_IDX (U must already be MSG_SHARED_U).
; Modifies: .A
MSG_RING_SELECT:
            lda         ZP_MSG_IDX
            and         #$1F                                ; (receiver & 1) * $10 + sender
            ora         #>MSG_RING_BASE
            sta         ZP_MSG_PTR + 1
            lda         ZP_MSG_IDX
            lsr                                             ; receiver >> 1
            lsr
            lsr
            lsr
            lsr
            clc
            adc         #MSG_RING_BANK
            sta         RAM_BANK_REG
            rts
