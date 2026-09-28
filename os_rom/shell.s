.debuginfo
.segment "SHELL"

SHELL_MAIN:
            jsr     IO_STD_OPEN         ; fds 0-2 on /dev/cons (inherited by the tasks the shell starts)
            jsr     COPYTORAM
            jsr     forth_main          ; (No clear screen: boot messages, e.g. a driver's FAIL, stay)
            jsr     MON_START