; ****************************************************************************
; fgetc.s - int __fastcall__ fgetc (FILE* f): a byte from its fd's buffer (hyfile.h), which _hy_fill fills when
; it's empty: so a file's bytes come a buffer at a time, a console's a line at a time.  In assembly: it's called
; for every byte (fgets, getc, scanf), and cc65's gets keeps a pointer in ptr4 across it (through fgets).  (It
; takes the place of cc65's, which read a byte a call.)

            .export     _fgetc
            .import     __hy_fill, __hy_fbuf
            .importzp   ptr1, ptr2, ptr4

            .include    "_file.inc"
            .include    "stdio.inc"

FB_BUF          = 0             ; hyfile.h's struct hy_fbuf (8 bytes): the buffer ...
FB_N            = 2             ;   the bytes in it ...
FB_AT           = 3             ;   the next one's place ...
FB_MODE         = 4             ;   reading (FB_READ) or writing
FB_READ         = 1

            .code

_fgetc:
            sta         ptr1
            stx         ptr1 + 1
@again:
            ldy         #_FILE::f_flags
            lda         (ptr1),Y
            and         #_FOPEN | _FERROR | _FEOF
            cmp         #_FOPEN
            bne         @eof
            lda         (ptr1),Y
            bit         #_FPUSHBACK
            bne         @back
            lda         (ptr1)                              ; Its fd's buffer (_FILE::f_fd: at fd * 8)
            asl
            asl
            asl
            tax
            lda         __hy_fbuf + FB_MODE,X
            cmp         #FB_READ
            bne         @fill
            ldy         __hy_fbuf + FB_AT,X                 ; A byte there?
            tya
            cmp         __hy_fbuf + FB_N,X
            beq         @fill
            inc         __hy_fbuf + FB_AT,X                 ; buf[at++]
            lda         __hy_fbuf + FB_BUF,X
            sta         ptr2
            lda         __hy_fbuf + FB_BUF + 1,X
            sta         ptr2 + 1
            lda         (ptr2),Y
            ldx         #0
            rts

@back:
            and         #<~_FPUSHBACK                       ; ungetc's byte
            sta         (ptr1),Y
            .assert     _FILE::f_pushback = _FILE::f_flags + 1, error, "fgetc.s: f_pushback after f_flags"
            iny
            lda         (ptr1),Y
            ldx         #0
            rts

@fill:
            lda         ptr4                                ; The buffer filled (ptr4 and f kept); then again
            pha                                             ;   (none: _FEOF or _FERROR is set)
            lda         ptr4 + 1
            pha
            lda         ptr1 + 1
            pha
            lda         ptr1
            pha
            ldx         ptr1 + 1
            jsr         __hy_fill
            pla
            sta         ptr1
            pla
            sta         ptr1 + 1
            pla
            sta         ptr4 + 1
            pla
            sta         ptr4
            bra         @again

@eof:
            lda         #<EOF
            tax
            rts
