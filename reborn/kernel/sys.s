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
            php
            sei
@task:
            QL_GET      TK_STATE
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
; The RAM modules (daughter cards: module m has banks $m0-$mF in every task): which are installed.  At boot, in
; the kernel task, IRQs off.  A missing module's banks float: a pattern written doesn't read back.
; OUT: K0_MODMASK (bit = module), K0_MODCOUNT.  RAM_BANK is left 0
K_PROBE:
            stz         K0_MODMASK
            stz         K0_MODMASK + 1
            stz         K0_MODCOUNT
            ldx         #RAM_MODULES - 1
@module:
            txa
            asl
            asl
            asl
            asl
            sta         RAM_BANK                            ; Its first bank
            lda         #$55
            sta         BANK_WINDOW
            cmp         BANK_WINDOW
            bne         @missing
            asl                                             ; ($AA)
            sta         BANK_WINDOW
            cmp         BANK_WINDOW
            bne         @missing
            inc         K0_MODCOUNT
            sec
            bra         @bit

@missing:
            clc
@bit:
            rol         K0_MODMASK                          ; (Module 14 first: it ends in bit 14)
            rol         K0_MODMASK + 1
            dex
            bpl         @module
            stz         RAM_BANK
            rts

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

; DBG_PS: a line for each task in use: "T STATE PARENT NAME" (state, flags and parent as hex)
K_DBG_PS:
            KPRINT      K_STR_PSHEAD
            ldx         #0
@task:
            ldy         T_REGISTER
            php
            sei
            QL_GET      TK_STATE
            sta         K_A
            QL_GET      TK_FLAGS
            sta         K_Y
            stz         T_REGISTER
            lda         K_PARENT,X
            sty         T_REGISTER
            plp
            sta         K_TMP
            lda         K_A
            beq         @next                               ; (Free)
            txa
            jsr         K_PUTNIB
            lda         #' '
            jsr         K_PUTC
            lda         K_A
            jsr         K_PUTHEX
            lda         #' '
            jsr         K_PUTC
            lda         K_Y
            jsr         K_PUTHEX
            lda         #' '
            jsr         K_PUTC
            lda         K_TMP
            jsr         K_PUTHEX
            lda         #' '
            jsr         K_PUTC
            stx         K_TASK                              ; Its name (from its TA_NAME: a copy, 16 bytes)
            cpx         T_REGISTER
            bne         @copy
            lda         #<TA_NAME                           ; (Ours: here)
            sta         r0
            lda         #>TA_NAME
            sta         r0 + 1
            bra         @print

@copy:
            lda         #<K_NAME
            sta         K_PTR
            lda         #>K_NAME
            sta         K_PTR + 1
            lda         #<TA_NAME
            sta         K_PTR2
            lda         #>TA_NAME
            sta         K_PTR2 + 1
            lda         #16
            sta         K_CNT
            stz         K_CNT + 1
            txa
            sec
            jsr         K_KCOPY
            stz         K_NAME + 15
            lda         #<K_NAME
            sta         r0
            lda         #>K_NAME
            sta         r0 + 1
@print:
            jsr         K_PUTSTR
            KPRINT      K_STR_PSEOL
            ldx         K_TASK
@next:
            inx
            cpx         #TASKS
            beq         :+
            jmp         @task
:
            clc
            rts

K_NAME          = TA_SCRATCH                                ; (A task's name, for a moment)

.segment "KRODATA"
K_STR_ERRNUM:   .byte   "error $"
K_STR_ERRNUM_END:
K_STR_PSHEAD:   .byte   "T ST FL PA NAME", CR, LF, 0
K_STR_PSEOL:    .byte   CR, LF, 0
