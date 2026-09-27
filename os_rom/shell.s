.debuginfo
.segment "SHELL"

SHELL_MAIN:
            jsr     IO_STD_OPEN         ; fds 0-2 on /dev/cons (inherited by the tasks the shell starts)
            jsr     COPYTORAM
            jsr     CLEAR_SCR
            jsr     forth_main
            jsr     MON_START