; ****************************************************************************
; common.s - the COMMON block: the same code at the same address ($FD00) on every BIOS ROM page, so it keeps
; running when W changes under it (the next instruction is fetched from the new page, at the same address).
; Only interrupt entry and exit, and the kernel's own calls between its pages, need it: everything outside the
; kernel runs with W = 0 (principle P2).
;   IRQ_STUB_0 ... IRQ_STUB_F   each line's vector points at its stub: .A = the line, on to IRQ_ENTRY
;   IRQ_ENTRY                   the frame's X and W, then page 0 and the dispatcher (irq.s)
;   IRQ_EXIT                    back to the interrupted page, and RTI
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
.repeat 16, I
            CLABEL      .ident(.sprintf("IRQ_STUB_%X", I))
            pha
            lda         #I
.if I < 15
            jmp         IRQ_ENTRY
.endif
.endrepeat                                                  ; (The last runs on into IRQ_ENTRY)

; .A = the line.  The frame so far: A, then the CPU's P and PC
            CLABEL      IRQ_ENTRY
            phx
            ldx         W_REGISTER
            phx
            stz         W_REGISTER                          ; Page 0 from here (this same code)
            jmp         IRQ_DISPATCH

; The frame's W, X and A, then RTI (the dispatcher and the scheduler come here on page 0)
            CLABEL      IRQ_EXIT
            pla
            sta         W_REGISTER                          ; Back on the interrupted page (this same code)
            plx
            pla
            rti

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
