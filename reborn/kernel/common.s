; ****************************************************************************
; common.s - the COMMON block: the same code at the same address ($FD00) on every BIOS ROM page, so it keeps
; running when W changes under it (the next instruction is fetched from the new page, at the same address).
; Only interrupt entry and exit need it: everything outside the kernel runs with W = 0 (principle P2).
;   IRQ_STUB_0 ... IRQ_STUB_F   each line's vector points at its stub: .A = the line, on to IRQ_ENTRY
;   IRQ_ENTRY                   the frame's X and W, then page 0 and the dispatcher (irq.s)
;   IRQ_EXIT                    back to the interrupted page, and RTI
;   NMI_ENTRY                   page 0's NMI_HANDLER, and back
; Labels are defined by page 0's copy; the others are checked to line up with it.

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
.endmacro

.segment "COMMON_P0"
            COMMON_BLOCK
common_define .set 0
.segment "COMMON_P1"
            COMMON_BLOCK
.segment "COMMON_P2"
            COMMON_BLOCK
.segment "COMMON_P3"
            COMMON_BLOCK
.segment "COMMON_P4"
            COMMON_BLOCK
.segment "COMMON_P5"
            COMMON_BLOCK
.segment "COMMON_P6"
            COMMON_BLOCK
.segment "COMMON_P7"
            COMMON_BLOCK
.segment "COMMON_P8"
            COMMON_BLOCK
.segment "COMMON_P9"
            COMMON_BLOCK
.segment "COMMON_PA"
            COMMON_BLOCK
.segment "COMMON_PB"
            COMMON_BLOCK
.segment "COMMON_PC"
            COMMON_BLOCK
.segment "COMMON_PD"
            COMMON_BLOCK
.segment "COMMON_PE"
            COMMON_BLOCK
.segment "COMMON_PF"
            COMMON_BLOCK
