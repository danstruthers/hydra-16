; ****************************************************************************
; crt0.s - a C program's start and end (cc65; hydra.cfg).  The program is a RAM program: SPAWN's loader reads its
; image (this header, then the rest, from $0800) into its task, clears its BSS, sets its break at HX_TOP, and
; starts it at HX_MAIN with r0 = its arguments (TASK_ARGS: zero-terminated strings, an empty one after the last).
; The header has no name: the loader names the task after its file.  Its RAM: $0800 to __RAMTOP__ (hydra.cfg): the
; program, its BSS, the heap (malloc), and the C stack, down from __RAMTOP__.
;   It ends when main returns, or exit(), _exit() or hy_exits() is called: the destructors (atexit's functions too),
; then EXITS with the exit status: its code (the status's low byte) and a message (hy_exits's, or none), which rc
; keeps ($status) and a waiting task gets (WAIT: hy_wait, system).  Its files are closed by the kernel.

            .export     _exit
            .export     __STARTUP__ : absolute = 1
            .export     __hy_args, __hy_exitmsg
            .import     initlib, donelib, callmain
            .import     __MAIN_START__, __DATA_LOAD__, __DATA_RUN__, __DATA_SIZE__
            .import     __BSS_RUN__, __BSS_SIZE__, __RAMTOP__

            .include    "zeropage.inc"
            .include    "hydra.inc"

; The HYX2 header (sdk/asm/hyx2.inc's, for cc65's segments): a program, a RAM program's (no HF_INPLACE)
            .segment    "HEADER"
            .byte       "HYX2"                              ; HX_MAGIC
            .byte       HX_SIZE                             ; HX_HSIZE
            .byte       HT_PROGRAM                          ; HX_TYPE
            .byte       0                                   ; HX_FLAGS
            .byte       ABI_VERSION                         ; HX_ABI
            .word       __MAIN_START__                      ; HX_LOAD ($0800)
            .word       __DATA_RUN__ + __DATA_SIZE__ - __MAIN_START__ ; HX_LENGTH (to its data's end)
            .word       __DATA_LOAD__                       ; HX_DATA_LOAD
            .word       __DATA_RUN__                        ; HX_DATA_RUN
            .word       __DATA_SIZE__                       ; HX_DATA_LEN
            .word       __BSS_RUN__                         ; HX_BSS
            .word       __BSS_SIZE__                        ; HX_BSS_LEN
            .word       __RAMTOP__                          ; HX_TOP
            .word       start                               ; HX_MAIN
            .word       0, 0, 0                             ; HX_SERVE, HX_IRQ, HX_STOP
            .byte       0                                   ; HX_DEVICE
            .byte       1                                   ; HX_BANKS
            .word       1                                   ; HX_VERSION
            .res        12, 0                               ; HX_NAME: none (its file's)

            .segment    "STARTUP"

start:
            cld
            lda         r0                                  ; Its arguments (TASK_ARGS: mainargs.s makes argc
            sta         __hy_args                           ;   and argv from them)
            lda         r0 + 1
            sta         __hy_args + 1
            lda         #<__RAMTOP__                        ; The C stack, down from the top of its RAM
            sta         sp
            lda         #>__RAMTOP__
            sta         sp + 1
            jsr         initlib                             ; The constructors: argc and argv, the current
            jsr         callmain                            ;   directory ...; then main (argc, argv)

; void __fastcall__ _exit (int status) (and exit: cc65's ends here): the destructors, then EXITS with the status's
; low byte and __hy_exitmsg (hy_exits's message; 0: none)
_exit:
            pha
            jsr         donelib
            lda         __hy_exitmsg
            sta         r0
            lda         __hy_exitmsg + 1
            sta         r0 + 1
            pla
            jmp         EXITS

            .data
__hy_args:  .word       0                                   ; (In DATA: set before the constructors run)
__hy_exitmsg: .word     0
