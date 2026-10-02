; ****************************************************************************
; call.s - running code in another task, and copying between tasks (docs/reimplementation-from-scratch.md, §10.4).
;
; SCALL runs a task's serve entry (TA_SERVEVEC) in that task: its zero page, its stack (below its frame), its
; banks (its module at $A000).  The caller is ST_CALL meanwhile; the called task is ST_READY, so the call can be
; switched out like any code.  A task serves one call at a time (TK_BUSY): a caller that finds it busy (serving
; another, or a driver still starting) is ST_BLOCKED on it, and looks again once the scheduler sees it free.
; Registers cross the switch: .A and .X in (.Y = the caller in the entry), .A, .X and C out (C rides in P:
; nothing on the way back changes it).
;
; KCALL is an SCALL into the kernel task (task 0), to a routine of the kernel's.  The kernel task is never
; preempted (its TK_PREEMPT is always at least 1), so KCALLs run one at a time, to their ends, and the kernel's
; tables need no locks (only IRQs off where an irq entry shares them).
;
; kcopy moves bytes between this task's memory and another's, each side as that task sees it (its own RAM, its
; RAM bank, its paged ROM bank): with T = the source for a byte's read and T = the destination for its write,
; each task's pointer in its own zero page (KC_PTR).  IRQs are off for a burst of 4 bytes at most.

.include "kdefs.inc"

.segment "KCODE"

; SCALL: task .Y's serve entry, in task .Y.  IN: .Y = the task, .A, .X = its arguments; IRQs on.
; OUT: .A, .X and C from the entry; or C = 1, .A = E_SRCH (not a task), E_NODEV (it's free, ended, or serves no
; calls: a program)
K_SCALL:
            cpy         T_REGISTER
            bne         @other
            ldy         TK_BUSY                             ; Ourselves: a plain call (if we serve)
            bmi         @nodev
            ldy         T_REGISTER
            jmp         (TA_SERVEVEC)

@srch:
            FAIL        E_SRCH

@gone:                                                      ; (In the task called; .A = this task)
            sta         T_REGISTER
            cli
@nodev:
            FAIL        E_NODEV

@busy:                                                      ; (In the task called; .A = this task; N: TK_BUSY's)
            bmi         @gone                               ; (BUSY_NOSERVE: it serves no calls)
            ldx         TK_INX                              ; Busy: blocked on it, till the scheduler sees it free.
            ldy         T_REGISTER                          ;   .X: the argument, .Y: the task called, for the
            sta         T_REGISTER                          ;   next look.  ---- This task
            stx         K_X
            sty         TK_BLOCKEDON
            lda         #ST_BLOCKED
            sta         TK_STATE
            jsr         K_YIELD                             ; (Comes back with IRQs off: a moment on, after the
            cli                                             ;   switch, before the next look)
            nop
            sei
            ldx         K_X
            bra         @look

@other:
            cpy         #TASKS
            bcs         @srch
            sei
            sta         K_A                                 ; (The argument .A, while we look)
@look:                                                      ; IRQs off; .X: the argument.  Is it there, and free?
            lda         T_REGISTER                          ; .A = this task (the caller)
            sty         T_REGISTER                          ; ---- The task called: its zero page (a quick look)
            stx         TK_INX                              ; (The argument .X, there already: a call it's serving
            ldx         TK_STATE                            ;   took its own at its start)
            beq         @gone                               ; (ST_FREE)
            ldx         TK_BUSY
            bne         @busy                               ; (Serving another call, starting, or serving none)
            inc         TK_BUSY
            sta         TK_INCALLER
            sta         T_REGISTER                          ; ---- This task: ST_CALL, its stack pointer saved
            ldx         #ST_CALL
            stx         TK_STATE
            tsx
            stx         TK_SP                               ; (Its interrupts run below here meanwhile)
            lda         K_A
            sty         T_REGISTER                          ; ---- The task called: its stack, below its frame
            ldx         TK_SP
            txs
            tax                                             ; (.X: the argument .A, a moment)
            lda         TK_STATE
            pha                                             ; Its state, back after the call
            lda         #ST_READY                           ; (The call runs: it can be switched out)
            sta         TK_STATE
            txa                                             ; .A, .X: the arguments; .Y: the caller
            ldx         TK_INX
            ldy         TK_INCALLER
            cli
            jsr         @entry
            sei                                             ; ---- Back from the entry, still in the task called
            stx         TK_INX                              ;   (.A, the result, stays in .A; C rides in P)
            ply
            sty         TK_STATE                            ; Its state before the call
            tsx
            stx         TK_SP                               ; Its stack pointer, as it was before the call
            stz         TK_BUSY                             ; Free again (callers blocked on it: the scheduler)
            ldy         TK_INCALLER
            ldx         TK_INX
            sty         T_REGISTER                          ; ---- The caller: its zero page (not its stack yet)
            stx         K_X
            ldx         TK_SP
            txs
            ldx         #ST_READY                           ; (A caller was running: it was ready)
            stx         TK_STATE
            ldx         TK_DUE                              ; A switch came due, and we don't hold the CPU: now
            beq         :+
            ldx         TK_PREEMPT
            bne         :+
            jsr         K_YIELD                             ; (It keeps every register and flag)
:
            ldx         K_X
            cli
            rts

@entry:
            jmp         (TA_SERVEVEC)

; KCALL (the macro in kdefs.inc names the routine in K_FNVEC and K_FNPAGE, then comes here): .A, .X = the arguments
K_KCALL:
            ldy         #KERNEL_TASK
            jmp         K_SCALL

; The kernel task's serve entry: the routine the caller (.Y) named, on its page (a far call, page 0's too)
K_KDISPATCH:
            pha
            phx
            php
            sei
            sty         T_REGISTER                          ; ---- The caller: the routine it named
            lda         K_FNVEC
            ldx         K_FNVEC + 1
            stz         T_REGISTER                          ; ---- Back
            sta         KF_VEC
            stx         KF_VEC + 1
            sty         T_REGISTER                          ; ---- The caller: its page
            lda         K_FNPAGE
            stz         T_REGISTER                          ; ---- Back
            sta         KF_PAGE
            plp
            plx
            pla
            jmp         K_FAR

; ****************************************************************************
; kcopy: K_CNT bytes between K_PTR (this task) and K_PTR2 (task .A); C = 0 from here to there, C = 1 from there
; to here.  IN: IRQs on; task .A not running, nor in a kcopy of its own.  OUT: C = 0.  Modifies .A, .X, .Y,
; K_TMP, K_TMP2, K_TASK, K_CNT.  (Both tasks' U is the one in effect now: a buffer in a shared bank works when
; they agree.)
;   Bursts of 4 bytes, unrolled (each byte: T to one task, then the other; .X = the task to switch to, read from
; the zero page we're in); then the 0-3 left, a byte at a time.  .Y indexes both ends; at its wrap, both ends'
; pointers move on a page.
K_KCOPY:
            sta         K_TASK
            lda         #0
            rol
            sta         K_TMP2                              ; The direction: 0 here to there, 1 there to here
            php
            sei
            ldx         K_TASK
            ldy         T_REGISTER
            lda         K_PTR2                              ; Its end: its pointer, and its partner: us
            QL_PUT      KC_PTR
            lda         K_PTR2 + 1
            QL_PUT      KC_PTR + 1
            tya
            QL_PUT      KC_PARTNER
            lda         K_PTR                               ; Our end
            sta         KC_PTR
            lda         K_PTR + 1
            sta         KC_PTR + 1
            stx         KC_PARTNER
            plp
            lda         K_CNT                               ; K_TMP: the bytes after the bursts (0-3) ...
            and         #3
            sta         K_TMP
            lsr         K_CNT + 1                           ; ... K_CNT: the bursts, as a count for "dec low;
            ror         K_CNT                               ;   bne; dec high; bne" (its high byte one up
            lsr         K_CNT + 1                           ;   unless the low is 0)
            ror         K_CNT
            ldy         #0
            lda         K_CNT
            ora         K_CNT + 1
            bne         :+
            jmp         @rest
:
            lda         K_CNT
            beq         :+
            inc         K_CNT + 1
:
            lda         K_TMP2
            bne         @in

@out:                                                       ; ---- Here to there, 4 bytes
            php
            sei
            .repeat     4
            lda         (KC_PTR),Y                          ; Ours ...
            ldx         KC_PARTNER
            stx         T_REGISTER
            sta         (KC_PTR),Y                          ; ... to its
            ldx         KC_PARTNER
            stx         T_REGISTER
            iny
            .endrepeat
            plp                                             ; (A moment for interrupts)
            tya
            bne         :+
            jsr         @page
:
            dec         K_CNT
            bne         @out
            dec         K_CNT + 1
            bne         @out
            bra         @rest

@in:                                                        ; ---- There to here, 4 bytes
            php
            sei
            .repeat     4
            ldx         KC_PARTNER
            stx         T_REGISTER
            lda         (KC_PTR),Y                          ; Its ...
            ldx         KC_PARTNER
            stx         T_REGISTER
            sta         (KC_PTR),Y                          ; ... to ours
            iny
            .endrepeat
            plp
            tya
            bne         :+
            jsr         @page
:
            dec         K_CNT
            bne         @in
            dec         K_CNT + 1
            bne         @in

@rest:                                                      ; ---- The 0-3 left, a byte at a time
            lda         K_TMP
            beq         @done
            php
            sei
            lda         K_TMP2
            bne         @rest_in
            lda         (KC_PTR),Y
            ldx         KC_PARTNER
            stx         T_REGISTER
            sta         (KC_PTR),Y
            ldx         KC_PARTNER
            stx         T_REGISTER
            bra         @rest_next

@rest_in:
            ldx         KC_PARTNER
            stx         T_REGISTER
            lda         (KC_PTR),Y
            ldx         KC_PARTNER
            stx         T_REGISTER
            sta         (KC_PTR),Y
@rest_next:
            plp
            iny
            dec         K_TMP
            bra         @rest

@done:
            clc
            rts

@page:                                                      ; A page done: both ends on a page
            inc         KC_PTR + 1
            ldx         KC_PARTNER
            php
            sei
            lda         T_REGISTER
            stx         T_REGISTER
            inc         KC_PTR + 1
            sta         T_REGISTER
            plp
            rts
