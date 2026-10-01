; ****************************************************************************
; oserrlist.s - the Hydra's errors (os_rom/include/kernel.inc) as text, for cc65's _stroserror (string.h) and
; _poserror (stdio.h): _stroserror (_oserror) says what the last failed call's error was, as HyForth's !IO ERR!
; does ("not found").  The table is cc65's: an entry's length, its code and its text; then one of length 0, for a
; code it hasn't got.

        .export     __sys_oserrlist

        .include    "hydra.inc"

.macro  oserr       code, msg
        .local      Start, End
Start:  .byte       End - Start
        .byte       code
        .asciiz     msg
End:
.endmacro

        .rodata

__sys_oserrlist:
        oserr       ERR_OUT_OF_MEMORY, "out of memory"
        oserr       ERR_IO_NOT_FOUND, "not found"
        oserr       ERR_IO_BAD_FD, "bad fd"
        oserr       ERR_IO_MODE, "not opened for that"
        oserr       ERR_IO_WOULD_BLOCK, "would wait"
        oserr       ERR_IO_EOF, "end of file"
        oserr       ERR_IO_NO_FDS, "no fds left"
        oserr       ERR_IO_NO_DEVS, "device table full"
        oserr       ERR_IO_NAME, "bad name"
        oserr       ERR_IO_BAD_REQ, "not supported"
        oserr       ERR_IO_DEVICE, "no answer"
        oserr       ERR_IO_BROKEN, "broken pipe"
        oserr       ERR_IO_NO_PIPES, "no pipes left"
        oserr       ERR_IO_NS_FULL, "namespace full"
        oserr       ERR_IO_NS_LOOP, "bind loop"
        oserr       ERR_IO_NOT_READY, "not ready"
        oserr       ERR_IO_MEDIA, "media error"
        oserr       ERR_IO_NOT_FS, "no HydraFS"
        oserr       ERR_IO_FULL, "disk full"
        oserr       ERR_IO_EXISTS, "exists"
        oserr       ERR_IO_NOT_EMPTY, "not empty"
        oserr       ERR_IO_BUSY, "busy"
        oserr       ERR_IO_NOT_DIR, "not a directory"
        oserr       ERR_IO_IS_DIR, "a directory"
        oserr       ERR_IO_NOT_EXEC, "not executable"
        oserr       ERR_IO_PERM, "not allowed"
        oserr       ERR_SEM_BAD, "no such semaphore"
        oserr       ERR_SEM_NONE, "no semaphores left"
        oserr       ERR_SEM_BUSY, "none to take"
        oserr       ERR_SEM_NOT_HELD, "not held"
        oserr       ERR_SEM_FULL, "count full"
        oserr       ERR_NO_TASKS_AVAILABLE, "no tasks left"
        oserr       ERR_TASK_BUSY, "task busy"
        oserr       ERR_BAD_TASK, "bad task"
        .byte       0, 0                    ; (Not one of them)
        .asciiz     "unknown error"
