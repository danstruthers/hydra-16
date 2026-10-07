; ****************************************************************************
; common.s - the COMMON block: the same code at the same address ($FD00) on every BIOS ROM page, so it keeps
; running when W changes under it (the next instruction is fetched from the new page, at the same address).
; Only interrupt entry and exit, and the kernel's own calls between its pages, need it: everything outside the
; kernel runs with W = 0 (principle P2).
;   IRQ_STUB_0 ... IRQ_STUB_F   each line's vector points at its stub: .A = the line, on to IRQ_ENTRY (but the
;                               VIA's at IRQ_VIA: the ACIA's first, then timer 2 and CA1, lines of their own,
;                               LINE_VIA_T2 and LINE_VIA_CA1)
;   IRQ_ENTRY                   the frame's X and W, then page 0 and the dispatcher (irq.s): its main path is
;                               here, to save two jumps on every interrupt
;   IRQ_RESTORE, IRQ_EXIT       back to the interrupted page, and RTI
;   NMI_ENTRY                   page 0's NMI_HANDLER, and back
;   K_FAR, K_FAR_GO             the kernel's far call (FARCALL: kdefs.inc)
;   K_PEEK_PAGE                 a byte on another page (POST's test of the W lines)
; Labels are defined by page 0's copy; the others are checked to line up with it.  After the block, at $FDFF,
; each page has its number.

.include "kdefs.inc"

common_define .set 1

.macro CLABEL   name
.if common_define
name:
.else
.assert     * = name, lderror, "The COMMON block's copies don't line up"
.endif
.endmacro

.macro COMMON_BLOCK
.local      stubs
stubs:
.repeat 16, I
            CLABEL      .ident(.sprintf("IRQ_STUB_%X", I))
.if I = LINE_VIA
            jmp         IRQ_VIA                             ; (Its vector is IRQ_VIA's own: IRQ_INIT)
            .res        3                                   ; (A stub's 6 bytes)
.else
            pha
            lda         #I
            jmp         IRQ_ENTRY
.endif
.endrepeat

; The VIA's line (its vector: IRQ_INIT): timer 2's interrupt (its own on, and run out) is LINE_VIA_T2's, CA1's
; LINE_VIA_CA1's, the rest the VIA's (.A = 0: the tick).  But the ACIA's first, if it's interrupting too (its stub's,
; as if it had come alone): the board gives the VIA's line the higher priority, and at 115200 a byte in has 311
; cycles before the next overruns it, too few to wait for the tick's and timer 2's.  (Its status read clears its
; interrupt: its handler looks at RDRF.  The VIA's comes again after it.)
            CLABEL      IRQ_VIA
            bit         ACIA_STATUS
            bmi         stubs + LINE_ACIA * 6               ; (IRQ_STUB_1: this copy's)
            pha
            lda         VIA_IFR
            and         VIA_IER
            bit         #VIA_IRQ_T2
            bne         :+
            and         #VIA_IRQ_CA1
            beq         :++                                 ; (.A = 0.  IRQ_ENTRY: this copy's)
            lda         #LINE_VIA_CA1
            bra         :++
:
            lda         #LINE_VIA_T2                        ; (Then on into IRQ_ENTRY: timer 2's sending, every
                                                            ;   character, the shortest way)

; .A = the line.  The frame so far: A, then the CPU's P and PC
:
            CLABEL      IRQ_ENTRY
            phx
            ldx         W_REGISTER
            phx
            stz         W_REGISTER                          ; Page 0 from here (this same code)

; The dispatcher (irq.s), here to save its jumps: .A = the line.  The stack: the frame's W, X, A, P, PCL, PCH
            phy                                             ; (The frame's Y)
            tay                                             ; .Y = the line
            tsx
            stx         TK_SP                               ; The interrupted task's stack pointer, for the way back
            ldx         TA_OWNERS,Y                         ; .X = the line's owner
            bmi         :++                                 ; (Nobody's: IRQ_STRAY)
            lda         T_REGISTER                          ; .A = the interrupted task
            stx         T_REGISTER                          ; ---- The owner: its zero page and stack page (not S yet)
            ldx         TK_SP
            txs                                             ; Its stack: below its frame (or the same, if it's the
            pha                                             ;   interrupted task).  The interrupted task, for later
            tax                                             ; .X = the interrupted task
            tya                                             ; .A = the line
            jsr         IRQ_HANDLER
            ply                                             ; ---- Back to the interrupted task
            sty         T_REGISTER
            ldx         TK_SP
            txs
            and         #IRQ_RESCHED                        ; A task switch, please?
            beq         :+
            ldx         TK_PREEMPT                          ; Not while it holds the CPU (or is in the scheduler):
            beq         :+++                                ;   then it's noted, for PREEMPT_ON or the next YIELD
            sta         TK_DUE                              ;   (.A = IRQ_RESCHED: not 0).  Else IRQ_SWITCH

; The end of an interrupt with no task switch (IRQ_STRAY's too): the frame's Y ...
:
            CLABEL      IRQ_RESTORE
            ply

; ... and W, X and A, then RTI (the scheduler comes here on page 0)
            CLABEL      IRQ_EXIT
            pla
            sta         W_REGISTER                          ; Back on the interrupted page (this same code)
            plx
            pla
            rti

:
            jmp         IRQ_STRAY
:
            jmp         IRQ_SWITCH

            CLABEL      NMI_ENTRY
            pha
            lda         W_REGISTER
            pha
            stz         W_REGISTER                          ; Page 0 (this same code)
            jsr         NMI_HANDLER
            pla
            sta         W_REGISTER                          ; Back (this same code)
            pla
            rti

; The kernel's far call (FARCALL): routine KF_VEC on page KF_PAGE, then back to the caller's page.  .A, .X, .Y
; and C pass both ways (N and Z don't).  KF_* are taken before the routine runs, so it can make far calls too
            CLABEL      K_FAR
            sta         KF_A                                ; (.A, a moment)
            lda         W_REGISTER
            pha                                             ; The caller's page, for the way back
            lda         KF_PAGE
            sta         W_REGISTER                          ; ---- The routine's page (this same code)
            lda         KF_A
            jsr         K_FAR_GO
            sta         KF_A
            pla
            sta         W_REGISTER                          ; ---- Back (this same code)
            lda         KF_A
            rts

            CLABEL      K_FAR_GO
            jmp         (KF_VEC)

; .A = the byte at (KF_VEC) on page .X.  Keeps .X, .Y
            CLABEL      K_PEEK_PAGE
            lda         W_REGISTER
            pha
            stx         W_REGISTER                          ; ---- Page .X (this same code)
            lda         (KF_VEC)
            sta         KF_A
            pla
            sta         W_REGISTER                          ; ---- Back (this same code)
            lda         KF_A
            rts
.endmacro

.repeat 16, P
.segment .sprintf("COMMON_P%X", P)
            COMMON_BLOCK
common_define .set 0
.segment .sprintf("ID_P%X", P)
            .byte       P                                   ; (This page's number)
.endrepeat
