; ****************************************************************************
; hycall.s - int __fastcall__ hy_call (unsigned call, struct hy_regs* regs): any system call, by its address (its
; slot in the jump table: hydracalls.h's HY_NAME), with its registers in regs (hydra.h: .A, .X, .Y, r0-r3), in and
; out.  0 (C = 0); or the error code (C = 1), _oserror and errno set (___mappederrno).

            .export     _hy_call
            .import     popax, ___mappederrno

            .include    "zeropage.inc"
            .include    "hydra.inc"

HR_A            = 0             ; struct hy_regs: a, x, y, r[4]
HR_X            = 1
HR_Y            = 2
HR_R            = 3

            .code

_hy_call:
            sta         ptr1                                ; The registers
            stx         ptr1 + 1
            jsr         popax                               ; The call
            sta         vec
            stx         vec + 1
            ldy         #HR_R                               ; r0-r3 in (a byte at a time, no loop: some 130
                                                            ;   cycles a call less, in and out, a poll's every
                                                            ;   call: 2026-11-16)
            lda         (ptr1),Y
            sta         r0
            iny
            lda         (ptr1),Y
            sta         r0 + 1
            iny
            lda         (ptr1),Y
            sta         r0 + 2
            iny
            lda         (ptr1),Y
            sta         r0 + 3
            iny
            lda         (ptr1),Y
            sta         r0 + 4
            iny
            lda         (ptr1),Y
            sta         r0 + 5
            iny
            lda         (ptr1),Y
            sta         r0 + 6
            iny
            lda         (ptr1),Y
            sta         r0 + 7
            ldy         #HR_Y                               ; .Y, .X, .A in
            lda         (ptr1),Y
            pha
            ldy         #HR_X
            lda         (ptr1),Y
            tax
            ldy         #HR_A
            lda         (ptr1),Y
            ply
            jsr         go
            php                                             ; .A, .X, .Y out
            sta         tmp1
            stx         tmp2
            sty         tmp3
            ldy         #HR_A
            sta         (ptr1),Y
            iny
            lda         tmp2
            sta         (ptr1),Y
            iny
            lda         tmp3
            sta         (ptr1),Y
            ldy         #HR_R                               ; r0-r3 out
            lda         r0
            sta         (ptr1),Y
            iny
            lda         r0 + 1
            sta         (ptr1),Y
            iny
            lda         r0 + 2
            sta         (ptr1),Y
            iny
            lda         r0 + 3
            sta         (ptr1),Y
            iny
            lda         r0 + 4
            sta         (ptr1),Y
            iny
            lda         r0 + 5
            sta         (ptr1),Y
            iny
            lda         r0 + 6
            sta         (ptr1),Y
            iny
            lda         r0 + 7
            sta         (ptr1),Y
            plp
            bcs         @error
            lda         #0
            tax
            rts

@error:
            lda         tmp1                                ; (_oserror, errno)
            jsr         ___mappederrno
            lda         tmp1
            ldx         #0
            rts

go:
            jmp         (vec)

            .bss
vec:        .res        2
