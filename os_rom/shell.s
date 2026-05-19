.debuginfo
.segment "SHELL"

SHELL_MAIN:
            jsr     COPYTORAM
            jsr     CLEAR_SCR
            jsr     forth_main
            jsr     MON_START