.debuginfo

; ****************************************************************************
; BIOS ROM page 1 (W = 1): HyForth and the disassembler.
;
;   This file is included inside `.scope PAGE1` (see all.s), BEFORE disasm.s and hyforth, so the gate
;   labels below take precedence over the page 0 routines of the same name for all page 1 code.
;   Page 1 code (including HyForth's RAM code) must run with W = 1; it's entered through the page 0
;   gates in page0_gates.s.

.segment "GATES_P1"

; Gates from page 1 to page 0 BIOS routines
FAR_GATE        READ_CHAR,      ::READ_CHAR,            0
FAR_GATE        WRITE_CHAR,     ::WRITE_CHAR,           0
FAR_GATE        WRITE_BYTE,     ::WRITE_BYTE,           0
FAR_GATE        WRITE_HEX,      ::WRITE_HEX,            0
FAR_GATE        WRITE_HEX_MASK, ::WRITE_HEX_MASK,       0
FAR_GATE        WRITE_CRLF,     ::WRITE_CRLF,           0
FAR_GATE        CLEAR_SCR,      ::CLEAR_SCR,            0
FAR_GATE        MEM_COPY,       ::MEM_COPY,             0
FAR_GATE        MM_ALLOC,       ::MM_ALLOC,             0
FAR_GATE        MM_FREE,        ::MM_FREE,              0
FAR_GATE        MM_READ,        ::MM_READ,              0
FAR_GATE        MM_WRITE,       ::MM_WRITE,             0
FAR_GATE        MM_LOCK,        ::MM_LOCK,              0
FAR_GATE        MM_UNLOCK,      ::MM_UNLOCK,            0
FAR_GATE        MMU_TEST,       ::MMU_TEST,             0

; Sound routines run in the sound task (through the page 0 SND_CALL_* task gates)
FAR_GATE        SOUND_INIT,     ::SND_CALL_INIT,        0
FAR_GATE        SOUND_TEST,     ::SND_CALL_TEST,        0
FAR_GATE        YM_WRITE,       ::SND_CALL_YM_WRITE,    0

FAR_JMP_GATE    MON_START,      ::MON_START,            0

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

; Page 1 copy of the BIOS thunk table, at the same address as the page 0 one ($F800), so code running
; on page 1 (e.g. Forth's syscall) can use the documented thunk addresses.
.segment "BIOS_THUNKS_P1"
.assert     * = ::TH_READ_CHAR, lderror, "BIOS_THUNKS_P1 must line up with BIOS_THUNKS"
                jmp             READ_CHAR           ; $F800
                jmp             WRITE_CHAR          ; $F803
                jmp             WRITE_BYTE          ; $F806
                jmp             WRITE_HEX           ; $F809
                jmp             WRITE_HEX_MASK      ; $F80C
                jmp             WRITE_HSTRING       ; $F80F
                jmp             WRITE_CRLF          ; $F812
                jmp             CLEAR_SCR           ; $F815
                jmp             DISASM              ; $F818
                jmp             DISASM_AY           ; $F81B
                jmp             MEM_COPY            ; $F81E
                jmp             MM_ALLOC            ; $F821
                jmp             MM_FREE             ; $F824
                jmp             MM_READ             ; $F827
                jmp             MM_WRITE            ; $F82A
                jmp             MM_LOCK             ; $F82D
                jmp             MM_UNLOCK           ; $F830
                jmp             MMU_TEST            ; $F833
.assert     * = ::TH_MMU_TEST + 3, lderror, "BIOS_THUNKS_P1 must match BIOS_THUNKS"
