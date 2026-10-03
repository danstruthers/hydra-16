; ****************************************************************************
; sys.s - the small calls: what the system has, error texts, the RAM modules' probe, and the debugging calls.

.include "kdefs.inc"

.segment "KCODE"

; SYSINFO: .A = the ABI version; .X = the RAM modules installed; r0 = the free tasks (bit = task)
K_SYSINFO:
            stz         r0
            stz         r0 + 1
            ldx         #TASKS - 1
            ldy         T_REGISTER
@task:
            php                                             ; (IRQs off for each look alone: all 16 at once
            sei                                             ;   held them off for 478 cycles)
            QL_GET      TK_STATE
            plp
            cmp         #ST_FREE + 1                        ; (C = 0: free)
            rol         r0
            rol         r0 + 1
            dex
            bpl         @task
            lda         r0                                  ; (Each bit came in as "used")
            eor         #$FF
            sta         r0
            lda         r0 + 1
            eor         #$FF
            sta         r0 + 1
            php
            sei
            K0_GET      K0_MODCOUNT
            plp
            tax
            lda         #ABI_VERSION
            clc
            rts

; ERRSTR: error .A's text into the 32 bytes at r0 (zero-terminated; "error $xx" for a code it doesn't know)
K_ERRSTR:
            sta         K_A
            lda         #<ERR_TEXTS
            sta         K_PTR
            lda         #>ERR_TEXTS
            sta         K_PTR + 1
@entry:
            lda         (K_PTR)
            beq         @unknown                            ; (The table's end)
            cmp         K_A
            beq         @found
@skip:
            jsr         @next                               ; Past its code and text
            lda         (K_PTR)
            bne         @skip
            jsr         @next
            bra         @entry

@found:
            jsr         @next
            ldy         #0
@copy:
            lda         (K_PTR),Y
            sta         (r0),Y
            beq         @done
            iny
            cpy         #31
            bne         @copy
            lda         #0
            sta         (r0),Y
@done:
            clc
            rts

@unknown:
            ldy         #0
:
            lda         K_STR_ERRNUM,Y
            sta         (r0),Y
            iny
            cpy         #K_STR_ERRNUM_END - K_STR_ERRNUM
            bne         :-
            lda         K_A
            lsr
            lsr
            lsr
            lsr
            jsr         @digit
            lda         K_A
            jsr         @digit
            lda         #0
            sta         (r0),Y
            clc
            rts

@digit:
            and         #$0F
            cmp         #10
            bcc         :+
            adc         #'A' - '0' - 10 - 1                 ; (C = 1)
:
            adc         #'0'
            sta         (r0),Y
            iny
            rts

@next:
            inc         K_PTR
            bne         :+
            inc         K_PTR + 1
:
            rts

; A spare slot in the jump table
K_NOSYS:
            FAIL        E_NOSYS

; ****************************************************************************
; The debugging calls (dbg: for the tests; unstable).  DBG_SCALL is SCALL itself (call.s)

; DBG_KCOPY: r2 bytes between r0 (here) and r1 (in task .A); C = 0 here to there, C = 1 there to here
K_DBG_KCOPY:
            php
            cmp         #TASKS
            bcs         @srch
            tax
            lda         r0
            sta         K_PTR
            lda         r0 + 1
            sta         K_PTR + 1
            lda         r1
            sta         K_PTR2
            lda         r1 + 1
            sta         K_PTR2 + 1
            lda         r2
            sta         K_CNT
            lda         r2 + 1
            sta         K_CNT + 1
            txa
            plp
            jmp         K_KCOPY

@srch:
            plp
            FAIL        E_SRCH

.segment "KRODATA"
K_STR_ERRNUM:   .byte   "error $"
K_STR_ERRNUM_END:
