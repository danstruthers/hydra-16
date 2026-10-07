; ****************************************************************************
; conglue.s - conio's assembly (conio.c has the rest): the character out (PUTC: fd 1, as stdout's output goes, so
; the two keep their order), and the entries cc65's own conio code calls: cputc, cgetc and gotoxy (cputs, cprintf,
; cscanf ...) and screensize (screensize()).  The common code keeps its pointers in the zero page (ptr1-ptr3, tmp1)
; across those calls, as a target's assembly leaves them alone; conio.c's C doesn't, so these keep them for it.

            .export     __hy_putc, _cputc, _cgetc, gotoxy, screensize
            .import     __hy_cputc, __hy_cgetc, _gotoxy, __hy_consize, popa
            .import     __hy_fbuf, __hy_flush, _stdout

            .include    "zeropage.inc"
            .include    "hydra.inc"

FB_STDOUT       = 1 * 8         ; hyfile.h: fd 1's buffer's record ...
FB_N            = 2             ;   its bytes waiting

; Keep the common code's zero page (on the stack: 7 bytes), and put it back
.macro KEEP_ZP
            lda         tmp1
            pha
            lda         ptr1
            pha
            lda         ptr1 + 1
            pha
            lda         ptr2
            pha
            lda         ptr2 + 1
            pha
            lda         ptr3
            pha
            lda         ptr3 + 1
            pha
.endmacro

.macro BACK_ZP
            pla
            sta         ptr3 + 1
            pla
            sta         ptr3
            pla
            sta         ptr2 + 1
            pla
            sta         ptr2
            pla
            sta         ptr1 + 1
            pla
            sta         ptr1
            pla
            sta         tmp1
.endmacro

            .code

; void __fastcall__ _hy_putc (char c): out, as it is (PUTC); what stdout has waiting in its buffer goes out first
__hy_putc:
            ldx         __hy_fbuf + FB_STDOUT + FB_N
            beq         :+
            pha
            lda         _stdout
            ldx         _stdout + 1
            jsr         __hy_flush
            pla
:
            jmp         PUTC

; void __fastcall__ cputc (char c)
_cputc:
            tay
            KEEP_ZP
            tya
            jsr         __hy_cputc
            BACK_ZP
            rts

; char cgetc (void)
_cgetc:
            KEEP_ZP
            jsr         __hy_cgetc
            tay
            BACK_ZP
            tya
            ldx         #0
            rts

; gotoxy: x and y on the C stack (cputsxy's, cputcxy's ...): y popped, then _gotoxy (.A = y, x on the stack)
gotoxy:
            jsr         popa
            tay
            KEEP_ZP
            tya
            jsr         _gotoxy
            BACK_ZP
            rts

; screensize: .X = the screen's width, .Y = its height.  (cc65's screensize () keeps its pointers in ptr1 and ptr2
; across it: conio.c's C, which reads the environment, doesn't keep them)
screensize:
            KEEP_ZP
            jsr         __hy_consize                        ; (.A = width, .X = height)
            sta         size_w
            stx         size_h
            BACK_ZP
            ldx         size_w
            ldy         size_h
            rts

            .bss
size_w:     .res        1
size_h:     .res        1
