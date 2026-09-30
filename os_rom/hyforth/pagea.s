.debuginfo

; ****************************************************************************
; BIOS ROM page A (W = $A): HyForth's far words (farwords.s) and the disassembler.  They run in the shell's
; task (HyForth's), called through page 1's gates (FW_CALL ...) and page 0's (DISASM ...).
;
;   This file is included first in `.scope FAR` (farwords.s), inside PAGE1, so the gate labels below take
;   precedence over page 1's routines and the page 0 routines of the same name for all page A code.

.segment "GATES_PA"

; Gates to page 0.  WRITE_CHAR and GET_CHAR take their stdio fast paths here, as on page 1 (page1.s)
WRITE_CHAR:
                    _M_STDOUT_FAST
FAR_GATE_INLINE     WRITE_CHAR_P0,  ::WRITE_CHAR,           0
GET_CHAR:
                    _M_STDIN_FAST
FAR_GATE_INLINE     GET_CHAR_P0,    ::GET_CHAR,             0
FAR_GATE_INLINE     IO_FLUSH,       ::IO_FLUSH,             0   ; (For WRITE_CHAR's fast path)
FAR_GATE_INLINE     WRITE_BYTE,     ::WRITE_BYTE,           0
FAR_GATE_INLINE     WRITE_HEX,      ::WRITE_HEX,            0
FAR_GATE_INLINE     WRITE_CRLF,     ::WRITE_CRLF,           0
FAR_GATE_INLINE     CLEAR_SCR,      ::CLEAR_SCR,            0
FAR_GATE_INLINE     TASK_RUN,       ::TASK_RUN,             0
FAR_GATE_INLINE     TASK_SLEEP,     ::TASK_SLEEP,           0
FAR_GATE_INLINE     TASK_SIGNAL,    ::TASK_SIGNAL,          0
FAR_GATE_INLINE     CONS_SET_FG,    ::CONS_SET_FG,          0
FAR_GATE_INLINE     SEM_NEW,        PAGE5::SEM_NEW,         5   ; (The semaphore words)
FAR_GATE_INLINE     SEM_ACQUIRE,    PAGE5::SEM_ACQUIRE,     5
FAR_GATE_INLINE     SEM_TRY,        PAGE5::SEM_TRY,         5
FAR_GATE_INLINE     SEM_RELEASE,    PAGE5::SEM_RELEASE,     5
FAR_GATE_INLINE     SEM_FREE,       PAGE5::SEM_FREE,        5
FAR_GATE_INLINE     MM_ALLOC,       ::MM_ALLOC,             0
FAR_GATE_INLINE     MM_FREE,        ::MM_FREE,              0
FAR_GATE_INLINE     MM_LOCK,        ::MM_LOCK,              0
FAR_GATE_INLINE     MM_UNLOCK,      ::MM_UNLOCK,            0
FAR_GATE_INLINE     MM_FIND,        ::MM_FIND,              0

; ... to the IO layer (page 2)
FAR_GATE_INLINE     IO_OPEN,        PAGE2::IO_OPEN,         2
FAR_GATE_INLINE     IO_CLOSE,       PAGE2::IO_CLOSE,        2
FAR_GATE_INLINE     IO_READ,        PAGE2::IO_READ,         2
FAR_GATE_INLINE     IO_WRITE,       PAGE2::IO_WRITE,        2
FAR_GATE_INLINE     IO_GETC,        PAGE2::IO_GETC,         2
FAR_GATE_INLINE     IO_SEEK,        PAGE2::IO_SEEK,         2
FAR_GATE_INLINE     IO_CTL,         PAGE2::IO_CTL,          2
FAR_GATE_INLINE     IO_DUP2,        PAGE2::IO_DUP2,         2
FAR_GATE_INLINE     IO_PIPE,        PAGE2::IO_PIPE,         2
FAR_GATE_INLINE     IO_CREATE,      PAGE2::IO_CREATE,       2
FAR_GATE_INLINE     IO_MOUNT,       PAGE2::IO_MOUNT,        2
FAR_GATE_INLINE     IO_BIND,        PAGE2::IO_BIND,         2
FAR_GATE_INLINE     IO_UNMOUNT,     PAGE2::IO_UNMOUNT,      2
FAR_GATE_INLINE     IO_NS_LIST,     PAGE2::IO_NS_LIST,      2
FAR_GATE_INLINE     TASK_CLONE,     PAGE2::TASK_CLONE,      2

; ... to the shell (page 7; the aliases are in all.s)
FAR_GATE_INLINE     SH_CMD,         ::SH_CMD_P7,            7
FAR_GATE_INLINE     SH_CD,          ::SH_CD_P7,             7
FAR_GATE_INLINE     SH_PWD,         ::SH_PWD_P7,            7
FAR_GATE_INLINE     INSAVE,         ::SH_INSAVE_P7,         7
FAR_GATE_INLINE     SH_PROMPT,      ::SH_PROMPT_P7,         7   ; (The shell library's line hooks:
FAR_GATE_INLINE     SH_REDIR,       ::SH_REDIR_P7,          7   ;   LINE_START ... in farwords.s)
FAR_GATE_INLINE     SH_UNREDIR,     ::SH_UNREDIR_P7,        7

; Page A copy of WRITE_HSTRING: the HString has to be read from page A, where the caller's strings are
; (the disassembler's).  .A, .Y hold the addr of HString to write
; Clobbers .A, .Y; Preserves .X
WRITE_HSTRING:
                phx
                sta             ZP_HS_TEMP
                sty             ZP_HS_TEMP + 1
                lda             (ZP_HS_TEMP)                ; Length of HString
                beq             @done
                tax
                ldy             #0
@write_loop:
                iny
                PRINT_CHAR      {(ZP_HS_TEMP),Y}
                dex
                bne             @write_loop
@done:
                plx
                rts
