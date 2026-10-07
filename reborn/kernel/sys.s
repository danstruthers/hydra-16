; ****************************************************************************
; sys.s - the small calls: what the system has, error texts, far calls, the RAM modules' probe, and the debugging
; calls.

.include "kdefs.inc"

.segment "KCODE"

; REBOOT: the system started again, as the reset button does: IRQs off, the VIA's and ACIA's interrupts quieted
; (only the button resets the chips: the boot takes them as it finds them), the reset entry.  .A = REBOOT_HWTEST:
; "HWT!" in the kernel task's RAM first, which POST takes as a T typed
K_REBOOT:
            sei
            stz         T_REGISTER                          ; ---- The kernel task's RAM (no way back: no stack)
            and         #REBOOT_HWTEST
            beq         :+
            ldx         #3
@word:
            lda         K_STR_HWT,X
            sta         K_HWT_WORD,X
            dex
            bpl         @word
:
            lda         #$7F                                ; Every VIA interrupt off, and none pending
            sta         VIA_IER
            sta         VIA_IFR
            sta         ACIA_STATUS                         ; (The ACIA: a programmed reset)
            jmp         RESET_ENTRY

; XCALL: the routine at r15 in paged ROM bank r14 (its low byte), as the X16's jsrfar: the caller's bank register
; ($01) the routine's for the call, then back; .A, .X, .Y, the flags and r0-r13 pass through both ways.  Here on page
; 0, which the switch of $01 leaves where it is.  The stack, as the routine runs: its return (here), the caller's
; bank, the caller's return
K_XCALL:
            pha                                             ; (A byte for the caller's bank)
            php
            pha
            phx
            tsx                                             ; (S+1 X, S+2 A, S+3 P, S+4 the byte)
            lda         ROM_BANK
            sta         $0104,X
            lda         r14
            sta         ROM_BANK                            ; The routine's bank
            plx
            pla
            plp
            jsr         @go
            php                                             ; Back: the caller's bank, and the byte gone (P over
            pha                                             ;   it)
            phx
            tsx                                             ; (S+1 X, S+2 A, S+3 P, S+4 the bank)
            lda         $0104,X
            sta         ROM_BANK
            lda         $0103,X
            sta         $0104,X
            plx
            pla
            plp
            plp
            rts

@go:
            jmp         (r15)

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

.segment "KRODATA"
K_STR_HWT:      .byte   "HWT!"                              ; (REBOOT_HWTEST's word: post.s has it too)
