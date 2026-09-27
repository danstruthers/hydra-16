.debuginfo

; ****************************************************************************
; BIOS ROM page 2 (W = 2): the IO layer (io.s) and its self test (io_test.s).
;
;   This file is included inside `.scope PAGE2` (see all.s), BEFORE io.s, so the gate labels below take
;   precedence over the page 0 routines of the same name for all page 2 code.  Page 2 code runs with
;   W = 2; it's entered through the compact gates in io_p0.s (page 0) and page1.s (page 1).

.segment "GATES_P2"

; Gates from page 2 to page 0 routines
FAR_GATE_INLINE     TASK_CALL,      ::TASK_CALL,            0
FAR_GATE_INLINE     YIELD,          ::YIELD,                0
FAR_GATE_INLINE     IO_SRV_MAP,     ::IO_SRV_MAP,           0
FAR_GATE_INLINE     IO_SRV_UNMAP,   ::IO_SRV_UNMAP,         0
FAR_GATE_INLINE     WRITE_CHAR,     ::WRITE_CHAR,           0
FAR_GATE_INLINE     WRITE_BYTE,     ::WRITE_BYTE,           0
FAR_GATE_INLINE     WRITE_CRLF,     ::WRITE_CRLF,           0
FAR_GATE_INLINE     MM_ALLOC,       ::MM_ALLOC,             0
FAR_GATE_INLINE     MM_FREE,        ::MM_FREE,              0
FAR_GATE_INLINE     MM_LOCK,        ::MM_LOCK,              0
FAR_GATE_INLINE     MM_UNLOCK,      ::MM_UNLOCK,            0
FAR_GATE_INLINE     SERIAL_SET_CAPTURE, ::SERIAL_SET_CAPTURE, 0
FAR_GATE_INLINE     POST_PUTC,      ::POST_PUTC,            0
FAR_GATE_INLINE     MMU_PROBE_MODULES, ::MMU_PROBE_MODULES, 0

; Page 2 copy of WRITE_HSTRING: the HString has to be read from page 2, where the caller's strings are.
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
