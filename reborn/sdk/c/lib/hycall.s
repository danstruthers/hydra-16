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
HR_RN           = 8             ; (r0-r3: 8 bytes)

            .code

_hy_call:
            sta         ptr1                                ; The registers
            stx         ptr1 + 1
            jsr         popax                               ; The call
            sta         vec
            stx         vec + 1
            ldx         #0                                  ; r0-r3 in
            ldy         #HR_R
:
            lda         (ptr1),Y
            sta         r0,X
            iny
            inx
            cpx         #HR_RN
            bne         :-
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
            ldx         #0                                  ; r0-r3 out
            ldy         #HR_R
:
            lda         r0,X
            sta         (ptr1),Y
            iny
            inx
            cpx         #HR_RN
            bne         :-
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
