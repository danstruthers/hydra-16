; ****************************************************************************
; hyx.inc - a Hydra executable's header (the shell's run loads it: shell/run.s; the layout is in
; os_rom/include/shell.inc).  Use HYX_HEADER once in a program linked with hyx.cfg, giving its entry point.

.macro HYX_HEADER entry
            .import     __PROG_START__, __PROG_LAST__
            .pushseg
            .segment    "HYXHDR"
            .byte       "HYX1"                          ; Magic
            .word       __PROG_START__                  ; Load address
            .word       __PROG_LAST__ - __PROG_START__  ; Length of the code
            .word       entry                           ; Entry point
            .word       0                               ; Flags
            .word       0                               ; BSS: none (hyx.cfg has none: variables go in DATA)
            .word       0                               ; Top: the code's end
            .popseg
.endmacro
