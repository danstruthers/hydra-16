; ****************************************************************************
; mainargs.s - argc and argv (a constructor), from the program's arguments (crt0.s: __hy_args, its task's
; TASK_ARGS: zero-terminated strings, an empty one after the last, as rc gives them, each word one) and its name
; (argv[0]: its task's, TASKINFO's).  argv[argc] is NULL.  MAXARGS - 1 arguments at most.

            .constructor initmainargs, 24
            .import     __argc, __argv, __hy_args

            .include    "zeropage.inc"
            .include    "hydra.inc"

MAXARGS         = 32

            .segment    "ONCE"

initmainargs:
            jsr         GETPID                              ; argv[0]: its name (its task's)
            pha
            lda         #<info
            sta         r0
            lda         #>info
            sta         r0 + 1
            pla
            jsr         TASKINFO
            bcc         :+
            lda         #0
            sta         info + TI_NAME
:
            lda         #<(info + TI_NAME)
            sta         argv
            lda         #>(info + TI_NAME)
            sta         argv + 1
            lda         __hy_args                           ; The rest: each string, till the empty one
            sta         ptr1
            lda         __hy_args + 1
            sta         ptr1 + 1
            ldx         #1                                  ; (.X: argc so far)
@arg:
            ldy         #0
            lda         (ptr1),Y
            beq         @done
            cpx         #MAXARGS - 1
            bcs         @done
            txa                                             ; argv[.X] = ptr1
            asl
            tay
            lda         ptr1
            sta         argv,Y
            lda         ptr1 + 1
            sta         argv + 1,Y
            inx
            ldy         #0                                  ; Past it and its 0
:
            lda         (ptr1),Y
            iny
            cmp         #0
            bne         :-
            tya
            clc
            adc         ptr1
            sta         ptr1
            bcc         @arg
            inc         ptr1 + 1
            bra         @arg

@done:
            txa                                             ; argv[argc] = NULL
            asl
            tay
            lda         #0
            sta         argv,Y
            sta         argv + 1,Y
            stx         __argc
            sta         __argc + 1
            lda         #<argv
            sta         __argv
            lda         #>argv
            sta         __argv + 1
            rts

            .bss
info:       .res        TI_SIZE
argv:       .res        MAXARGS * 2
