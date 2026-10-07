; ****************************************************************************
; conglue.s - conio's assembly (conio.c has the rest): the character out (WRITE_CHAR: as stdout's output goes), and
; the entries cc65's common conio code calls: cputc, cgetc and gotoxy (cputs, cprintf, cscanf ...) and screensize
; (screensize()).  The common code keeps its own pointers in the zero page (ptr1-ptr3, tmp1) across those calls,
; as a target's assembly leaves them alone; conio.c's C doesn't, so these keep them for it.

        .export     __hy_putc, _cputc, _cgetc, gotoxy, screensize
        .import     __hy_cputc, __hy_cgetc, _gotoxy, __hy_consize, popa

        .include    "zeropage.inc"
        .include    "hydra.inc"

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

; void __fastcall__ _hy_putc (char c): out, as it is
__hy_putc:
        jmp         WRITE_CHAR

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

; screensize: .X = the screen's width, .Y = its height
screensize:
        jsr         __hy_consize                        ; (.A = width, .X = height)
        stx         tmp1
        tax
        ldy         tmp1
        rts
