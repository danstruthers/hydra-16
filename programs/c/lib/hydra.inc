; ****************************************************************************
; hydra.inc - the Hydra's calls for the C library (and for assembly in C programs): the thunks' addresses,
; the zero-page parameters some calls take, and the constants they use.  They come from the OS ROM
; (os_rom/kernel/thunks.s, include/zero.s, include/io.inc, include/kernel.inc) and don't move between ROM
; builds; sim/regress.js checks them against the ROM's.  See docs/programming/rom-layout.md (the API index).

; The calls (thunks, on ROM page 0: a program's entry point is called there)
READ_CHAR       = $F800     ; .A = a key, or 0: none (no wait)
WRITE_CHAR      = $F803     ; .A = a character to stdout
MM_ALLOC        = $F821
MM_FREE         = $F824
MM_SET_FLOOR    = $F851     ; .A = a page: the MMU hands out none below it
YIELD           = $F854
TASK_RUN        = $F863     ; .A.Y = an entry point, .X = its ROM page -> .A = the new task (it runs alongside)
TASK_SIGNAL     = $F8AB     ; .A = TASK_BREAK_FLAG or TASK_KILL_FLAG, .X = a task (and the tasks it started)
IO_OPEN         = $F86C     ; .A.Y = name, .X = mode -> .A = fd
IO_CLOSE        = $F86F     ; .A = fd
IO_READ         = $F872     ; .A = fd, ZP_IO_BUF, ZP_IO_CNT -> ZP_IO_CNT = bytes read (0: the end)
IO_WRITE        = $F875     ; .A = fd, ZP_IO_BUF, ZP_IO_CNT -> ZP_IO_CNT = bytes written
IO_SEEK         = $F87E     ; .A = fd, ZP_IO_OFS = 32-bit offset
IO_STAT         = $F881     ; .A = fd, ZP_IO_BUF = a 48-byte buffer (IO_STAT_SIZE)
IO_CTL          = $F884     ; .A = fd, .X = code, .Y = argument -> .A = the server's answer
GET_CHAR        = $F88D     ; -> .A = a key (waits; echoed)
IO_DUP2         = $F890     ; .A = fd, .X = another: .X is .A's file too (closed first, if it was open)
IO_PIPE         = $F893     ; -> .A = its reading end's fd, .X = its writing end's
IO_DUP          = $F896     ; .A = fd -> .A = a new fd for the same file
IO_CREATE       = $F8C9     ; .A.Y = name, .X = mode, ZP_IO_BUF (low byte) = its HFS_M_* bits -> .A = fd
IO_REMOVE       = $F8CC     ; .A.Y = name
IO_WSTAT        = $F8CF     ; .A = fd, ZP_IO_BUF = a stat record (rename, mode bits)
IO_CHDIR        = $F8D2     ; .A.Y = a directory
IO_GETCWD       = $F8D5     ; ZP_IO_BUF = a 64-byte buffer <- the current directory
SEM_NEW         = $F8D8     ; .A = count, .Y = 0 or SEM_MUTEX -> .A = semaphore
SEM_ACQUIRE     = $F8DB     ; .A = semaphore (waits)
SEM_TRY         = $F8DE     ; .A = semaphore -> ERR_SEM_BUSY: none
SEM_RELEASE     = $F8E1     ; .A = semaphore
SEM_FREE        = $F8E4     ; .A = semaphore
TASK_SLEEP      = $F8E7     ; .A.Y = ticks (TICKS_PER_SEC a second, up to 32767)
TICKS_GET       = $F8EA     ; -> .A.Y = the tick count
CLOCK_GET       = $F8ED     ; .X = a zero page address <- 4 bytes: seconds since 2000-01-01
TASK_EXITS      = $F8F0     ; .A = code (0: success), ZP_IO_BUF = message (high byte 0: none): the task ends
TASK_JOIN       = $F8F3     ; .A = a task it started: waits for its end -> .A = its code; its message to
                            ;   ZP_IO_BUF (EXIT_MSG_MAX + 1 bytes; high byte 0: not wanted)
SHELL_CMD       = $F8F6     ; An entry point for TASK_RUN (page 0): a command shell, running its stdin's lines
T_REGISTER      = $FFF0     ; (Read: the running task's number)

; The calls' zero-page parameters (fixed addresses: zero.s)
ZP_IO_BUF       = $06       ; (2) a buffer or a name
ZP_IO_CNT       = $08       ; (2) a byte count
ZP_IO_OFS       = $0A       ; (4) an offset

; Constants
TICKS_PER_SEC   = 200
IO_MODE_READ    = $01
IO_MODE_WRITE   = $02
IO_MODE_RDWR    = $03
IO_MODE_STAT    = $04       ; (A directory: its entries as stat records)
IO_MODE_TRUNC   = $08
IO_MODE_NONBLOCK = $80      ; (A read with nothing yet: ERR_IO_WOULD_BLOCK)
HFS_M_DIR       = $80       ; A new file's mode bits (IO_CREATE): a directory
IO_STAT_SIZE    = 48        ; A stat record (IO_STAT) ...
IO_ST_NAME      = 0         ;   its name (32 bytes, zero-terminated)
IO_ST_MODE      = 32        ;   HFS_M_* bits
IO_ST_CARD      = 33        ;   the card it's on
IO_ST_QID       = 36        ;   its id on the card (4 bytes)
IO_ST_SIZE      = 40        ;   its size (4 bytes)
IO_ST_STAMP     = 44        ;   its last change (4 bytes: seconds since 2000-01-01)
HFS_M_RO        = $01       ; (A mode bit: read-only)
IO_FD_TABLE     = $7DA0     ; The task's fds (the task system page): 8 bytes each ...
IO_FD_SIZE      = 8
IO_FD_OFS       = 4         ;   the offset (4 bytes)
IO_MAX_FDS      = 12
SEM_MUTEX       = $80
HYX_ARGS_SIZE   = 64        ; A program's arguments, at most (with the 0)
HYX_NAME        = $0150     ; Its name, as it was run (argv[0]), zero-terminated ...
HYX_NAME_SIZE   = 32        ;   at most (with the 0)
EXIT_MSG_MAX    = 30        ; An exit status's message: its characters, at most
TASK_BREAK_FLAG = $10       ; TASK_SIGNAL: a break (Ctrl-C) ...
TASK_KILL_FLAG  = $20       ;   or a kill (Ctrl-\, kill)
UNIX_2000       = 946684800 ; 2000-01-01 00:00:00 in seconds since 1970 (the clock counts from 2000)
SND_CTL_INIT    = 1         ; /dev/snd's IO_CTL codes: clear the chip and the library's settings
SND_CTL_CLAIM   = 4         ;   channels (a mask) this fd's alone
SND_CTL_RELEASE = 5         ;   give them back
SND_CTL_VOLUME  = 6         ;   the master volume (0-127)
SND_R_CH        = $02       ; /dev/snd's commands (a register/value pair each): the channel ...
SND_R_PATCH     = $03       ;   a patch into it
SND_R_NOTE      = $04       ;   key on a MIDI note
SND_R_OFF       = $05       ;   key off
SND_R_VOL       = $06       ;   its volume
SND_R_PAN       = $07       ;   its speakers
SND_R_BEND      = $09       ;   its bend (1/64 semitone)
SND_R_DRUM      = $0A       ;   a General MIDI drum

; OS errors (kernel.inc)
ERR_OUT_OF_MEMORY   = $02
ERR_MEM_NOT_VALID   = $41
ERR_MEM_BAD_ARG     = $43
ERR_SEM_BAD         = $60
ERR_SEM_NONE        = $61
ERR_SEM_BUSY        = $62
ERR_SEM_NOT_HELD    = $63
ERR_SEM_FULL        = $64
ERR_IO_NOT_FOUND    = $70
ERR_IO_BAD_FD       = $71
ERR_IO_MODE         = $72
ERR_IO_WOULD_BLOCK  = $73
ERR_IO_EOF          = $74
ERR_IO_NO_FDS       = $75
ERR_IO_NAME         = $77
ERR_IO_BAD_REQ      = $78
ERR_IO_DEVICE       = $79
ERR_IO_BROKEN       = $7A
ERR_IO_NOT_READY    = $7E
ERR_IO_MEDIA        = $7F
ERR_IO_NOT_FS       = $80
ERR_IO_FULL         = $81
ERR_IO_EXISTS       = $82
ERR_IO_NOT_EMPTY    = $83
ERR_IO_BUSY         = $84
ERR_IO_NOT_DIR      = $85
ERR_IO_IS_DIR       = $86
ERR_IO_NOT_EXEC     = $87
