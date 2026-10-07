.debuginfo

; ****************************************************************************
; BIOS ROM page 4 (W = 4): the self tests (mmu_test.s, sched_test.s, io_test.s, post_ram.s).
;
;   This file is included inside `.scope PAGE4` (see all.s), before the rest of page 4, so the gate labels
;   below take precedence over the page 0 routines of the same name for all page 4 code.  Page 4 code
;   runs with W = 4; it's entered through the compact gates in page0_gates.s and io_p0.s (page 0) and
;   page1.s (page 1).  Routines passed to TASK_CALL are named with :: (their page 0 addresses), because
;   TASK_CALL runs them on page 0.

.segment "GATES_P4"

; Gates from page 4 to page 0 routines
FAR_GATE_INLINE     WRITE_CHAR_BUF, ::WRITE_CHAR,           0
FAR_GATE_INLINE     IO_FLUSH,       PAGE2::IO_FLUSH,        2
FAR_GATE_INLINE     WRITE_BYTE,     ::WRITE_BYTE,           0
FAR_GATE_INLINE     WRITE_HEX,      ::WRITE_HEX,            0
FAR_GATE_INLINE     WRITE_CRLF,     ::WRITE_CRLF,           0
FAR_GATE_INLINE     MM_ALLOC,       ::MM_ALLOC,             0
FAR_GATE_INLINE     MM_FREE,        ::MM_FREE,              0
FAR_GATE_INLINE     MM_READ,        ::MM_READ,              0
FAR_GATE_INLINE     MM_WRITE,       ::MM_WRITE,             0
FAR_GATE_INLINE     MM_LOCK,        ::MM_LOCK,              0
FAR_GATE_INLINE     MM_UNLOCK,      ::MM_UNLOCK,            0
FAR_GATE_INLINE     MM_TASK_RESET,  ::MM_TASK_RESET,        0
FAR_GATE_INLINE     SH_ALLOC,       ::SH_ALLOC,             0
FAR_GATE_INLINE     SH_ATTACH,      ::SH_ATTACH,            0
FAR_GATE_INLINE     SH_DETACH,      ::SH_DETACH,            0
FAR_GATE_INLINE     SH_READ,        ::SH_READ,              0
FAR_GATE_INLINE     SH_WRITE,       ::SH_WRITE,             0
FAR_GATE_INLINE     SH_LOCK,        ::SH_LOCK,              0
FAR_GATE_INLINE     SH_UNLOCK,      ::SH_UNLOCK,            0
FAR_GATE_INLINE     TASK_CALL,      ::TASK_CALL,            0
FAR_GATE_INLINE     TASK_RUN,       ::TASK_RUN,             0
FAR_GATE_INLINE     TASK_STATUS,    ::TASK_STATUS,          0
FAR_GATE_INLINE     TASK_WAIT,      ::TASK_WAIT,            0
FAR_GATE_INLINE     IO_WAKE,        ::IO_WAKE,              0
FAR_GATE_INLINE     YIELD,          ::YIELD,                0
FAR_GATE_INLINE     NO_PREEMPT,     ::NO_PREEMPT,           0
FAR_GATE_INLINE     PREEMPT,        ::PREEMPT,              0
FAR_GATE_INLINE     MMU_PROBE_MODULES, ::MMU_PROBE_MODULES, 0

; WOZMON's (monitor/wozmon.s)
FAR_GATE_INLINE     GET_CHAR,       ::GET_CHAR,             0
FAR_GATE_INLINE     WRITE_PROMPT,   ::WRITE_PROMPT,         0
FAR_GATE_INLINE     SPAWN_TASK,     ::SPAWN_TASK,           0
FAR_GATE_INLINE     DISASM_WM,      ::DISASM_WM_PA,         $A

; Gates from page 4 to far pointers and references (page 5)
FAR_GATE_INLINE     FP_MAKE,        PAGE5::FP_MAKE,         5
FAR_GATE_INLINE     FP_READ,        PAGE5::FP_READ,         5
FAR_GATE_INLINE     FP_COPY,        PAGE5::FP_COPY,         5
FAR_GATE_INLINE     SH_FP,          PAGE5::SH_FP,           5   ; (The IO test's /dev/ram step)
FAR_GATE_INLINE     MM_REF,         PAGE5::MM_REF,          5
FAR_GATE_INLINE     MM_FP,          PAGE5::MM_FP,           5
FAR_GATE_INLINE     SH_REF,         PAGE5::SH_REF,          5

; Gates from page 4 to the IO layer (page 2)
FAR_GATE_INLINE     IO_OPEN,        PAGE2::IO_OPEN,         2
FAR_GATE_INLINE     IO_CLOSE,       PAGE2::IO_CLOSE,        2
FAR_GATE_INLINE     IO_READ,        PAGE2::IO_READ,         2
FAR_GATE_INLINE     IO_SEEK,        PAGE2::IO_SEEK,         2
FAR_GATE_INLINE     IO_WRITE,       PAGE2::IO_WRITE,        2
FAR_GATE_INLINE     IO_GETC,        PAGE2::IO_GETC,         2
FAR_GATE_INLINE     IO_PUTC,        PAGE2::IO_PUTC,         2
FAR_GATE_INLINE     IO_CTL,         PAGE2::IO_CTL,          2
FAR_GATE_INLINE     IO_DUP2,        PAGE2::IO_DUP2,         2
FAR_GATE_INLINE     IO_PIPE,        PAGE2::IO_PIPE,         2
FAR_GATE_INLINE     IO_MOUNT,       PAGE2::IO_MOUNT,        2
FAR_GATE_INLINE     IO_BIND,        PAGE2::IO_BIND,         2
FAR_GATE_INLINE     IO_UNMOUNT,     PAGE2::IO_UNMOUNT,      2

; The tests' output: each character written out at once (stdout is line-buffered for the console), so the
; scheduler test shows the tasks' turns as they happen.  Preserves .A, .X, .Y
WRITE_CHAR:
                jsr             WRITE_CHAR_BUF
                jmp             IO_FLUSH

; .A = the byte at (ZP_D_XAM) on BIOS ROM page ZP_D_PAGE (only $E000-$FDFF is paged; 0: the kernel's), through
; PEEK_PAGE (COMMON), with ZP_FP borrowed.  Preserves .X, .Y, C; N/Z reflect .A
PEEK_D_XAM:
                lda             ZP_D_XAM
                sta             ZP_FP
                lda             ZP_D_XAM + 1
                sta             ZP_FP + 1
                phy
                ldy             #0
                lda             ZP_D_PAGE
                jsr             PEEK_PAGE
                ply
                ora             #0
                rts

; Page 4 copy of WRITE_HSTRING: the HString has to be read from page 4, where the caller's strings are.
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
