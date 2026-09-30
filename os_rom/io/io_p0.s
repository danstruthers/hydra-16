.debuginfo

.segment "IO_P0"

; ****************************************************************************
; IO subsystem, the BIOS ROM page 0 part (see docs/plans/IO_PLAN.md; the IO layer itself is io.s, on page 2).
; Drivers (on page 0) use these: DEV_REGISTER, and IO_SRV_MAP / IO_SRV_UNMAP in their serve routines.

; The IO layer's calls, for page 0 callers and the $F8xx thunks (compact gates into page 2)
FAR_GATE_INLINE     IO_OPEN,        PAGE2::IO_OPEN,         2
FAR_GATE_INLINE     IO_CLOSE,       PAGE2::IO_CLOSE,        2
FAR_GATE_INLINE     IO_READ,        PAGE2::IO_READ,         2
FAR_GATE_INLINE     IO_WRITE,       PAGE2::IO_WRITE,        2
FAR_GATE_INLINE     IO_GETC,        PAGE2::IO_GETC,         2
FAR_GATE_INLINE     IO_PUTC,        PAGE2::IO_PUTC,         2
FAR_GATE_INLINE     IO_SEEK,        PAGE2::IO_SEEK,         2
FAR_GATE_INLINE     IO_STAT,        PAGE2::IO_STAT,         2
FAR_GATE_INLINE     IO_CTL,         PAGE2::IO_CTL,          2
FAR_GATE_INLINE     IO_TEST,        PAGE4::IO_TEST,         4   ; (The self test, on page 4)
FAR_GATE_INLINE     IO_DUP2,        PAGE2::IO_DUP2,         2
FAR_GATE_INLINE     IO_PIPE,        PAGE2::IO_PIPE,         2
FAR_GATE_INLINE     IO_DUP,         PAGE2::IO_DUP,          2
FAR_GATE_INLINE     TASK_CLONE,     PAGE2::TASK_CLONE,      2
FAR_GATE_INLINE     TASK_CLONE_PAGE, PAGE2::TASK_CLONE_PAGE, 2  ; (Run in the new task by TASK_CLONE)
FAR_GATE_INLINE     IO_MOUNT,       PAGE2::IO_MOUNT,        2
FAR_GATE_INLINE     IO_BIND,        PAGE2::IO_BIND,         2
FAR_GATE_INLINE     IO_UNMOUNT,     PAGE2::IO_UNMOUNT,      2
FAR_GATE_INLINE     NS_CLEAR_ALL,   PAGE2::NS_CLEAR_ALL,    2
FAR_GATE_INLINE     IO_NS_LIST,     PAGE2::IO_NS_LIST,      2
FAR_GATE_INLINE     IO_STD_OPEN,    PAGE2::IO_STD_OPEN,     2
FAR_GATE_INLINE     IO_CLOSE_ALL,   PAGE2::IO_CLOSE_ALL,    2
FAR_GATE_INLINE     IO_INHERIT,     PAGE2::IO_INHERIT,      2
FAR_GATE_INLINE     IO_ADOPT_FDS,   PAGE2::IO_ADOPT_FDS,    2  ; (Run in a new task by IO_INHERIT)
FAR_GATE_INLINE     IO_FLUSH,       PAGE2::IO_FLUSH,        2  ; stdio buffering (WRITE_CHAR, GET_CHAR,
FAR_GATE_INLINE     STDOUT_PUT,     PAGE2::STDOUT_PUT,      2  ;   READ_CHAR)
FAR_GATE_INLINE     STDIN_GET,      PAGE2::STDIN_GET,       2

; Serve routines must be page 0 addresses (TASK_CALL runs them on page 0): gates to the page 2 servers
FAR_GATE_INLINE     NULL_SERVE,     PAGE2::NULL_SERVE,      2
FAR_GATE_INLINE     ZERO_SERVE,     PAGE2::ZERO_SERVE,      2
FAR_GATE_INLINE     PROC_SERVE,     ::PROC_SERVE_P9,        9   ; /dev/proc (proc_srv.s)
FAR_GATE_INLINE     ENV_SERVE,      ::ENV_SERVE_P9,         9   ; env (env_srv.s: the shell registers it)
FAR_GATE_INLINE     PROC_MEM_COUNT, ::PROC_MEM_COUNT_P9,    9   ; (/dev/proc/N/mem: TASK_CALL, in task N)

NULL_NAME:  .byte   "null", 0
ZERO_NAME:  .byte   "zero", 0
PROC_NAME:  .byte   "proc", 0

; The pipe server (pipe_srv.s): a Resident task (PIPE_TASK_NUM), started at boot like the drivers
PIPE_DRIVER:
            .word       PIPE_INIT                           ; DriverInfo::init
            .word       PIPE_STOP                           ; DriverInfo::stop
            .word       PIPE_DNAME                          ; DriverInfo::name
NamedHString PIPE_DNAME, "PIPE"
PIPE_NAME:  .byte   "pipe", 0

FAR_GATE_INLINE     PIPE_SERVE,     PAGE2::PIPE_SERVE,      2

; Runs in the pipe task: no pipes yet, the rings (task RAM pages from the MMU, kept for good: page
; blocks don't move), and register /dev/pipe.  OUT: C = 0, or C = 1 and .A = error
PIPE_INIT:
            ldx         #PIPE_MAX * PIPE_ENTRY_SIZE - 1
:
            stz         PIPE_TABLE,X
            dex
            bpl         :-
            lda         #<(PIPE_MAX * $100)
            ldy         #>(PIPE_MAX * $100)
            ldx         #0
            jsr         MM_ALLOC
            bcs         @done
            jsr         MM_LOCK                             ; .A.Y = the address (page aligned)
            bcs         @done
            sty         PIPE_BUF_PAGE
            LOAD_ADDR   PIPE_SERVE, ZP_TC_VEC
            lda         #<PIPE_NAME
            ldy         #>PIPE_NAME
            ldx         #PIPE_TASK_NUM
            jmp         DEV_REGISTER

@done:
            rts

PIPE_STOP:
            clc
            rts

; Clear the tasks' namespaces, and register the IO layer's own devices.  Called at boot, after
; SHARED_RAM_INIT (which clears the device table).
IO_INIT:
            jsr         NS_CLEAR_ALL
            LOAD_ADDR   NULL_SERVE, ZP_TC_VEC
            lda         #<NULL_NAME
            ldy         #>NULL_NAME
            ldx         #IO_DEV_CALLER_TASK
            jsr         DEV_REGISTER
            LOAD_ADDR   ZERO_SERVE, ZP_TC_VEC
            lda         #<ZERO_NAME
            ldy         #>ZERO_NAME
            ldx         #IO_DEV_CALLER_TASK
            jsr         DEV_REGISTER
            LOAD_ADDR   PROC_SERVE, ZP_TC_VEC
            lda         #<PROC_NAME
            ldy         #>PROC_NAME
            ldx         #IO_DEV_CALLER_TASK
            jmp         DEV_REGISTER

; Register a device (a file server): /dev/<name> is served by the serve routine, running in a task.
; Drivers call it from their init (which runs in the driver's task).
; IN: .A.Y = name (zero-terminated, 1-8 characters; read as the caller sees it, through a far pointer: in
;     RAM, the paged ROM, or on its ROM page), .X = task the serve routine runs in (IO_DEV_CALLER_TASK = the
;     task making each request), ZP_TC_VEC = serve routine (page 0)
; OUT (success): .A = device index, C = 0
; OUT (failure): .A = ERR_IO_NAME (empty, too long, or unreadable) or ERR_IO_NO_DEVS, C = 1
; Preserves .X, .Y
; DEV_REGISTER is for page 0 code (and the thunk); other pages' gates go to DEV_REGISTER_FAR.
DEV_REGISTER_FAR:
            stx         ZP_IO_TMP                           ; The task
            pha
            tsx
            lda         $0104,X                             ; The caller's ROM page (FAR_CALL_A pushed it,
            tax                                             ;   under its return address)
            pla
            bra         DEV_REGISTER_NAME

DEV_REGISTER:
            stx         ZP_IO_TMP                           ; The task
            ldx         #0                                  ; (Page 0)

DEV_REGISTER_NAME:
            jsr         FP_MAKE                             ; ZP_FP = the name
            ldx         ZP_IO_TMP
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            _M_SYS_ENTER                                    ; Select shared bank ID $00 (device table)
            ldy         #0
            jsr         FP_READ                             ; (It maps the name's memory for each byte, and
            bcs         @empty_name                         ;   puts the device table back)
            cmp         #0
            beq         @empty_name
            ldx         #0                                  ; Device table offset

@find:
            lda         IO_DEV_TABLE + IO_DEV_NAME,X
            beq         @free                               ; Free entry
            txa
            clc
            adc         #IO_DEV_SIZE
            tax
            bcc         @find                               ; (16 entries x 16 bytes: ends at 256)
            lda         #ERR_IO_NO_DEVS
            bra         @fail

@free:
            stx         ZP_IO_CNT                           ; The entry's offset
            ldy         #0                                  ; Copy the name, zero-padded

@name:
            jsr         FP_READ
            bcs         @bad_name
            cmp         #0
            beq         @pad
            cpy         #IO_DEV_NAME_LEN
            bcs         @bad_name                           ; Too long
            sta         IO_DEV_TABLE + IO_DEV_NAME,X
            inx
            iny
            bra         @name

@pad:
            cpy         #IO_DEV_NAME_LEN
            bcs         @padded
            stz         IO_DEV_TABLE + IO_DEV_NAME,X
            inx
            iny
            bra         @pad

@padded:
            ldx         ZP_IO_CNT                           ; Back to the entry's start
            lda         ZP_IO_TMP
            sta         IO_DEV_TABLE + IO_DEV_TASK,X
            lda         ZP_TC_VEC
            sta         IO_DEV_TABLE + IO_DEV_SERVE,X
            lda         ZP_TC_VEC + 1
            sta         IO_DEV_TABLE + IO_DEV_SERVE + 1,X
            txa                                             ; Device index = offset / 16
            lsr
            lsr
            lsr
            lsr
            clc
            bra         @done

@bad_name:
            ldx         ZP_IO_CNT
            stz         IO_DEV_TABLE + IO_DEV_NAME,X        ; Leave the entry free

@empty_name:
            lda         #ERR_IO_NAME

@fail:
            sec

@done:
            _M_SYS_LEAVE
            PULL_YX
            jmp         MM_RETURN

; Remove every device a task registered (DRV_START: a driver whose init failed).  IN: .A = task
; Preserves .A, .X, .Y
DEV_UNREGISTER_TASK:
            php                                             ; Save caller's I flag
            sei
            PUSH_AXY
            sta         ZP_IO_TMP
            _M_SYS_ENTER                                    ; Select shared bank ID $00 (device table)
            ldx         #0                                  ; Device table offset

@entry:
            lda         IO_DEV_TABLE + IO_DEV_TASK,X
            cmp         ZP_IO_TMP
            bne         @next
            stz         IO_DEV_TABLE + IO_DEV_NAME,X        ; Free

@next:
            txa
            clc
            adc         #IO_DEV_SIZE
            tax
            bcc         @entry                              ; (16 entries x 16 bytes: ends at 256)
            _M_SYS_LEAVE
            PULL_YXA
            plp                                             ; Restore caller's I flag
            rts

; Server side: map the client's request block (in its IO transfer area) into $8000-$9FFF, in the
; server's task.  Undo with IO_SRV_UNMAP before returning from the serve routine.
; IN: .X = client task.  OUT: ZP_IO_REQ = the request block (data at ZP_IO_REQ + IO_BLK_DATA)
; Preserves .A, .X, .Y
IO_SRV_MAP:
            pha
            lda         RAM_BANK_REG
            sta         ZP_IO_SAVEB
            lda         U_REGISTER
            sta         ZP_IO_SAVEU
            stz         U_REGISTER
            lda         #IO_XFER_BANK
            sta         RAM_BANK_REG
            stz         ZP_IO_REQ
            txa
            and         #$0F
            asl                                             ; $8000 + task * $200
            ora         #>PAGED_RAM_BASE
            sta         ZP_IO_REQ + 1
            pla
            rts

; Undo IO_SRV_MAP.  Preserves .A, .X, .Y and C
IO_SRV_UNMAP:
            pha
            lda         ZP_IO_SAVEU
            sta         U_REGISTER
            lda         ZP_IO_SAVEB
            sta         RAM_BANK_REG
            pla
            rts
