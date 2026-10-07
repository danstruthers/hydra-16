.debuginfo

; ****************************************************************************
; BIOS ROM page 1 (W = 1): HyForth.  (Its far words and the disassembler are on page A, farwords.s; the
; self tests are on page 4, page4.s.)
;
;   This file is included inside `.scope PAGE1` (see all.s), BEFORE hyforth, so the gate labels below take
;   precedence over the page 0 routines of the same name for all page 1 code.  Page 1 code (HyForth's,
;   and the code user words compile to in RAM) must run with W = 1; it's entered through the page 0 gates
;   in page0_gates.s.

.segment "GATES_P1"

; Gates from page 1 to page 0 BIOS routines
; READ_CHAR, WRITE_CHAR and GET_CHAR take their stdio fast paths here, and only go to page 0 when they're
; not enough (see _M_STDOUT_FAST): a byte through a pipe needs no far call
READ_CHAR:
                _M_STDIN_FAST
FAR_GATE_INLINE READ_CHAR_P0,   ::READ_CHAR,            0
WRITE_CHAR:
                _M_STDOUT_FAST
FAR_GATE_INLINE WRITE_CHAR_P0,  ::WRITE_CHAR,           0
GET_CHAR:
                _M_STDIN_FAST
FAR_GATE_INLINE GET_CHAR_P0,    ::GET_CHAR,             0
FAR_GATE_INLINE IO_FLUSH,       ::IO_FLUSH,             0
FAR_GATE_INLINE WRITE_BYTE,     ::WRITE_BYTE,           0
FAR_GATE_INLINE WRITE_HEX,      ::WRITE_HEX,            0
FAR_GATE_INLINE WRITE_HEX_MASK, ::WRITE_HEX_MASK,       0
FAR_GATE_INLINE WRITE_CRLF,     ::WRITE_CRLF,           0
FAR_GATE_INLINE CLEAR_SCR,      ::CLEAR_SCR,            0
FAR_GATE_INLINE MEM_COPY,       ::MEM_COPY,             0
FAR_GATE_INLINE MM_ALLOC,       ::MM_ALLOC,             0
FAR_GATE_INLINE MM_FREE,        ::MM_FREE,              0
FAR_GATE_INLINE MM_READ,        ::MM_READ,              0
FAR_GATE_INLINE MM_WRITE,       ::MM_WRITE,             0
FAR_GATE_INLINE MM_LOCK,        ::MM_LOCK,              0
FAR_GATE_INLINE MM_UNLOCK,      ::MM_UNLOCK,            0
FAR_GATE_INLINE TASK_CALL,      ::TASK_CALL,            0
FAR_GATE_INLINE SH_ALLOC,       ::SH_ALLOC,             0
FAR_GATE_INLINE SH_ATTACH,      ::SH_ATTACH,            0
FAR_GATE_INLINE SH_DETACH,      ::SH_DETACH,            0
FAR_GATE_INLINE SH_READ,        ::SH_READ,              0
FAR_GATE_INLINE SH_WRITE,       ::SH_WRITE,             0
FAR_GATE_INLINE SH_LOCK,        ::SH_LOCK,              0
FAR_GATE_INLINE SH_UNLOCK,      ::SH_UNLOCK,            0
FAR_GATE_INLINE MM_TASK_RESET,  ::MM_TASK_RESET,        0
FAR_GATE_INLINE MM_FIND,        ::MM_FIND,              0
FAR_GATE_INLINE MM_SET_FLOOR,   ::MM_SET_FLOOR,         0
FAR_GATE_INLINE MM_TASK_INIT,   ::MM_TASK_INIT,         0   ; Reset the current task's MMU area (HyForth cold)
FAR_GATE_INLINE YIELD,          ::YIELD,                0
FAR_GATE_INLINE NO_PREEMPT,     ::NO_PREEMPT,           0
FAR_GATE_INLINE PREEMPT,        ::PREEMPT,              0
FAR_GATE_INLINE TASK_WAIT,      ::TASK_WAIT,            0
FAR_GATE_INLINE IO_WAKE,        ::IO_WAKE,              0
FAR_GATE_INLINE TASK_RUN,       ::TASK_RUN,             0
FAR_GATE_INLINE TASK_STATUS,    ::TASK_STATUS,          0
FAR_GATE_INLINE TASK_SLEEP,     ::TASK_SLEEP,           0

; Gates from page 1 to the IO layer (page 2) and DEV_REGISTER (page 0)
FAR_GATE_INLINE IO_OPEN,        PAGE2::IO_OPEN,         2
FAR_GATE_INLINE IO_CLOSE,       PAGE2::IO_CLOSE,        2
FAR_GATE_INLINE IO_READ,        PAGE2::IO_READ,         2
FAR_GATE_INLINE IO_WRITE,       PAGE2::IO_WRITE,        2
FAR_GATE_INLINE IO_GETC,        PAGE2::IO_GETC,         2
FAR_GATE_INLINE IO_PUTC,        PAGE2::IO_PUTC,         2
FAR_GATE_INLINE IO_SEEK,        PAGE2::IO_SEEK,         2
FAR_GATE_INLINE IO_STAT,        PAGE2::IO_STAT,         2
FAR_GATE_INLINE IO_CTL,         PAGE2::IO_CTL,          2
FAR_GATE_INLINE DEV_REGISTER,   ::DEV_REGISTER_FAR,     0   ; (Reads the name as page 1 sees it)
FAR_GATE_INLINE IO_DUP2,        PAGE2::IO_DUP2,         2
FAR_GATE_INLINE IO_PIPE,        PAGE2::IO_PIPE,         2
FAR_GATE_INLINE IO_DUP,         PAGE2::IO_DUP,          2
FAR_GATE_INLINE TASK_CLONE,     PAGE2::TASK_CLONE,      2
FAR_GATE_INLINE IO_MOUNT,       PAGE2::IO_MOUNT,        2
FAR_GATE_INLINE IO_BIND,        PAGE2::IO_BIND,         2
FAR_GATE_INLINE IO_UNMOUNT,     PAGE2::IO_UNMOUNT,      2
FAR_GATE_INLINE IO_NS_LIST,     ::PROC_NS_LIST_P9,      9
FAR_GATE_INLINE PROC_CMD,       ::PROC_CMD_P9,          9   ; (fbreak: a /proc/N/cmd line's wake-up?)
FAR_GATE_INLINE IO_CREATE,      PAGE2::IO_CREATE,       2
FAR_GATE_INLINE IO_REMOVE,      PAGE2::IO_REMOVE,       2
FAR_GATE_INLINE IO_WSTAT,       PAGE2::IO_WSTAT,        2
FAR_GATE_INLINE IO_CHDIR,       PAGE2::IO_CHDIR,        2
FAR_GATE_INLINE IO_GETCWD,      PAGE2::IO_GETCWD,       2
FAR_GATE_INLINE IO_STD_OPEN,    PAGE2::IO_STD_OPEN,     2   ; (A bare Forth's start: forth_bare_main)

; Gates to the shell's routines (page 7; the aliases are in all.s)
FAR_GATE_INLINE SH_CD,          ::SH_CD_P7,             7
FAR_GATE_INLINE SH_PWD,         ::SH_PWD_P7,            7
FAR_GATE_INLINE SH_CMD,         ::SH_CMD_P7,            7
FAR_GATE_INLINE INSAVE,         ::SH_INSAVE_P7,         7
FAR_GATE_INLINE TASK_SET_BREAK, ::TASK_SET_BREAK,       0
FAR_GATE_INLINE TASK_SIGNAL,    ::TASK_SIGNAL,          0
FAR_GATE_INLINE CONS_SET_FG,    ::CONS_SET_FG,          0

; HyForth's far words, and the routines with them, and the disassembler (page A, hyforth/farwords.s; the
; aliases are in all.s)
FAR_GATE_INLINE FW_CALL,        ::FW_ENTRY_PA,          $A  ; A far word (FARWORD)
FAR_GATE_INLINE LINE_START,     ::LINE_START_PA,        $A  ; (The shell library's part of reading a
FAR_GATE_INLINE LINE_PROMPT,    ::LINE_PROMPT_PA,       $A  ;   line: getline)
FAR_GATE_INLINE LINE_READ,      ::LINE_READ_PA,         $A
FAR_GATE_INLINE LINE_EOF,       ::LINE_EOF_PA,          $A  ; (Stdin's end; a task's end, with its status)
FAR_GATE_INLINE LINE_EXITS,     ::LINE_EXITS_PA,        $A
FAR_GATE_INLINE INCOPEN,        ::INCOPEN_PA,           $A
FAR_GATE_INLINE INCOPENFD,      ::INCOPENFD_PA,         $A
FAR_GATE_INLINE INCEND,         ::INCEND_PA,            $A
FAR_GATE_INLINE INCCOUNT,       ::INCCOUNT_PA,          $A
FAR_GATE_INLINE INCABORT,       ::INCABORT_PA,          $A
FAR_GATE_INLINE RUNNAME,        ::RUNNAME_PA,           $A
FAR_GATE_INLINE wrterror,       ::wrterror_PA,          $A
FAR_GATE_INLINE MALLOC,         ::MALLOC_PA,            $A
FAR_GATE_INLINE DISASM,         ::DISASM_PA,            $A
FAR_GATE_INLINE DISASM_AY,      ::DISASM_AY_PA,         $A

; Far pointers and references (page 5)
FAR_GATE_INLINE FP_MAKE,        PAGE5::FP_MAKE,         5
FAR_GATE_INLINE FP_READ,        PAGE5::FP_READ,         5
FAR_GATE_INLINE FP_WRITE,       PAGE5::FP_WRITE,        5
FAR_GATE_INLINE FP_COPY,        PAGE5::FP_COPY,         5
FAR_GATE_INLINE MM_REF,         PAGE5::MM_REF,          5
FAR_GATE_INLINE MM_FP,          PAGE5::MM_FP,           5
FAR_GATE_INLINE SH_REF,         PAGE5::SH_REF,          5
FAR_GATE_INLINE SH_FP,          PAGE5::SH_FP,           5

; The self tests (page 4)
FAR_GATE_INLINE MMU_TEST,       PAGE4::MMU_TEST,        4
FAR_GATE_INLINE SCHED_TEST,     PAGE4::SCHED_TEST,      4
FAR_GATE_INLINE IO_TEST,        PAGE4::IO_TEST,         4

FAR_JMP_GATE    MON_START,      ::MON_START_P4,         4

; Page 1 copy of WRITE_HSTRING: the HString has to be read from page 1, where the caller's strings are.
; .A, .Y hold the addr of HString to write
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

; Page 1's copy of the thunk table (include/thunks.inc), at the same address as page 0's ($F800), so code
; running on page 1 (HyForth's binary words) can use the documented thunk addresses.  An entry jumps to page
; 1's gate for the call; or, for the calls page 1 has no gate for (THUNK_P0), goes on to page 0's entry at
; the same address (THUNK_TO_P0).
.segment "BIOS_THUNKS_P1"
.macro THUNK name, addr
.assert     * = addr, lderror, .sprintf("Page 1's thunk %s isn't page 0's", .string(name))
                jmp             name
.endmacro
.macro THUNK_P0 name, addr
.assert     * = addr, lderror, .sprintf("Page 1's thunk %s isn't page 0's", .string(name))
.assert     <(* + 2) >= 2, error, "THUNK_TO_P0 takes 2 from the return address's low byte only"
                jsr             THUNK_TO_P0
.endmacro

                THUNK_TABLE
.delmacro       THUNK
.delmacro       THUNK_P0

; Page 1's thunk entries for the calls it has no gate for: on to page 0's entry at the same address (the
; entry's return address - 2), as a far call.  .A, .X, .Y, C and V pass through both ways (FAR_CALL_A); a
; name the call reads has to be in RAM or the paged ROM, not on page 1.
.segment "FORTH_TOP"
THUNK_TO_P0:
                sta             ZP_FAR_A
                pla                                 ; The entry's address + 2 (no carry: C and V go through)
                sta             ZP_FAR_VEC
                pla
                sta             ZP_FAR_VEC + 1
                dec             ZP_FAR_VEC
                dec             ZP_FAR_VEC
                stz             ZP_FAR_PAGE
                jmp             FAR_CALL_A
