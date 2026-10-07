; ****************************************************************************
; mainargs.s - argc and argv, from the program's arguments (crt0.s: hy_argp, the rest of the line after the
; program's name, zero-terminated) and its name (HYX_NAME: as it was run, for argv[0]).  The arguments are split
; at spaces; "a b" is one argument (the quotes left out).  argv[argc] is NULL.  At most MAXARGS - 1 arguments.

        .constructor initmainargs, 24
        .import     __argc, __argv, hy_argp

        .include    "zeropage.inc"
        .include    "hydra.inc"

MAXARGS = 16

        .segment    "ONCE"

initmainargs:
        ldy         #0                                  ; argv[0]: its name (copied: it's in the stack page)
@name:
        lda         HYX_NAME,y
        sta         name,y
        beq         @args
        iny
        cpy         #HYX_NAME_SIZE - 1
        bne         @name
        lda         #0
        sta         name,y

@args:
        lda         hy_argp                             ; A copy of the arguments too
        sta         ptr1
        lda         hy_argp + 1
        sta         ptr1 + 1
        ldy         #0
@copy:
        lda         (ptr1),y
        sta         argbuf,y
        beq         @split
        iny
        cpy         #HYX_ARGS_SIZE - 1
        bne         @copy
        lda         #0
        sta         argbuf,y

@split:
        lda         #<name
        sta         argv
        lda         #>name
        sta         argv + 1
        ldx         #1                                  ; .X = argc so far, .Y = where in argbuf
        ldy         #0

@skip:
        lda         argbuf,y                            ; Spaces between them
        beq         @done
        cmp         #' '
        bne         @word
        iny
        bne         @skip

@word:
        cpx         #MAXARGS - 1
        bcs         @done
        cmp         #'"'
        bne         @plain
        iny                                             ; "...": the quotes left out
        jsr         store
@quoted:
        lda         argbuf,y
        beq         @done
        cmp         #'"'
        beq         @end
        iny
        bne         @quoted

@plain:
        jsr         store
@chars:
        lda         argbuf,y
        beq         @done
        cmp         #' '
        beq         @end
        iny
        bne         @chars

@end:
        lda         #0                                  ; (Its end)
        sta         argbuf,y
        iny
        bne         @skip

@done:
        txa                                             ; argv[argc] = NULL
        asl
        tay
        lda         #0
        sta         argv,y
        sta         argv + 1,y
        stx         __argc
        sta         __argc + 1
        lda         #<argv
        sta         __argv
        lda         #>argv
        sta         __argv + 1
        rts

; argv[.X] = argbuf + .Y, and .X up by one.  Preserves .Y
store:
        sty         tmp1
        txa
        asl
        tay
        lda         tmp1
        clc
        adc         #<argbuf
        sta         argv,y
        lda         #>argbuf
        adc         #0
        sta         argv + 1,y
        ldy         tmp1
        inx
        rts

        .bss
name:       .res    HYX_NAME_SIZE
argbuf:     .res    HYX_ARGS_SIZE
argv:       .res    MAXARGS * 2
