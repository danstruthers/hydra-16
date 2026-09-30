; ****************************************************************************
; crt0.s - a C program's start and end (cc65, hydra.cfg).  The program is a Hydra executable: the shell's loader
; reads it into its own task at $0800, clears its BSS and keeps its RAM from the MMU (the header's HYX_BSS and
; HYX_TOP: as Plan 9's a.out header gives the kernel the bss's size), and calls its entry point (on ROM page 0,
; where the calls' thunks are) with .A.Y = its arguments, and its name at HYX_NAME (mainargs.s makes argc and argv
; from them).
;   Its RAM: $0800 to __RAMTOP__ (hydra.cfg): the program, its BSS, the heap (malloc), and the C stack
; (__STACKSIZE__, down from __RAMTOP__).  The MMU's allocations (MM_ALLOC) come from the pages above.
;   It ends when main returns, or exit() or hy_exits() is called: the destructors (atexit's functions too), then
; TASK_EXITS, with the exit status (the code, and a message: hy_exits's), which the shell keeps ($status, status)
; or a waiting task gets (hy_wait, system).  Its files are closed and its memory freed.

        .export     _exit
        .export     __STARTUP__ : absolute = 1
        .export     hy_argp, __hy_exitmsg
        .import     initlib, donelib, callmain
        .import     __MAIN_START__, __DATA_RUN__, __DATA_SIZE__, __BSS_RUN__, __BSS_SIZE__, __RAMTOP__

        .include    "zeropage.inc"
        .include    "hydra.inc"

; The executable's header (os_rom/include/shell.inc): the code and data are in the file; the BSS (INIT and BSS,
; after them) isn't: the loader clears it
        .segment    "HYXHDR"
        .byte       "HYX1"                              ; Magic
        .word       __MAIN_START__                      ; Load address
        .word       __DATA_RUN__ + __DATA_SIZE__ - __MAIN_START__   ; Length
        .word       start                               ; Entry point
        .word       0                                   ; Flags
        .word       __BSS_RUN__ + __BSS_SIZE__ - (__DATA_RUN__ + __DATA_SIZE__)    ; HYX_BSS
        .word       __RAMTOP__                          ; HYX_TOP: the MMU's floor

        .segment    "STARTUP"

start:
        cld
        sta         hy_argp                             ; The arguments (in the stack page, at HYX_ARGS:
        sty         hy_argp + 1                         ;   mainargs.s copies them before they're needed)
        lda         #<__RAMTOP__                        ; The C stack, down from the top of its RAM
        sta         sp
        lda         #>__RAMTOP__
        sta         sp + 1
        jsr         initlib                             ; The constructors: argc and argv, the current
        jsr         callmain                            ;   directory ...; then main (argc, argv)

; void __fastcall__ _exit (int status) (and exit: cc65's is this): the destructors, then the task ends with the
; exit status: its low byte, and __hy_exitmsg (hy_exits: a message; 0: none)
_exit:
        pha
        jsr         donelib
        lda         __hy_exitmsg
        sta         ZP_IO_BUF
        lda         __hy_exitmsg + 1
        sta         ZP_IO_BUF + 1
        pla
        jmp         TASK_EXITS

        .data
hy_argp:        .word   0                               ; (In DATA: set before the constructors run)
__hy_exitmsg:   .word   0
