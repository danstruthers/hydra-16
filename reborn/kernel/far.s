; ****************************************************************************
; far.s - system calls whose routines are on another BIOS ROM page.  Their jump table slots go to stubs on page
; 0 (made by tools/apigen.js from the calls marked "far" in spec/api.def):
;       FAR_NAME:   jsr K_FARJMP
;                   .byte <.bank(K_NAME)
;                   .word K_NAME
; K_FARJMP takes the page and address from after the jsr, and goes on to the kernel's far call (common.s: K_FAR),
; which returns to the program.  .A, .X, .Y and C pass both ways.  About 90 cycles more than a call on page 0.

.include "kdefs.inc"

.segment "KCODE"

K_FARJMP:
            sta         KF_A                                ; (.A, a moment)
            pla
            sta         KF_VEC                              ; KF_VEC: the stub's jsr's last byte (a moment)
            pla
            sta         KF_VEC + 1
            phy
            ldy         #1
            lda         (KF_VEC),Y
            sta         KF_PAGE                             ; The page ...
            iny
            lda         (KF_VEC),Y
            pha
            iny
            lda         (KF_VEC),Y
            sta         KF_VEC + 1                          ; ... and the routine
            pla
            sta         KF_VEC
            ply
            lda         KF_A
            jmp         K_FAR                               ; (It returns to the program, past the jump table)
