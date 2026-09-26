.debuginfo

; ****************************************************************************
; Code common to every BIOS ROM page (W = $0-$F)
;
;   The COMMON block is emitted into every ROM page at the same address ($FD00), so code in it keeps
;   running correctly across a write to W: the next instruction is fetched from the new page, at the
;   same address, and is the same instruction.  It holds:
;       * the 16 IRQ entry stubs (the IRQ vector RAM always points here) and the IRQ exit
;       * the NMI entry
;       * the far-call / far-jump trampolines used to call between ROM pages
;   Labels are defined by the page 0 copy; the other copies assert that they line up with it.

common_define .set 1

; Define a label in the page 0 copy of the common block; check the address in the other copies
.macro CLABEL   name
.if common_define
name:
.else
.assert     * = name, lderror, "COMMON block copies are misaligned"
.endif
.endmacro

.macro COMMON_BLOCK

; IRQ entry stubs, one per logical IRQ# (the vector RAM is indexed by IRQ_NUMBER(n))
.repeat 16, I
            CLABEL      .ident(.sprintf("IRQ_STUB_%X", I))
            pha
            lda         #I
            jmp         IRQ_ENTRY
.endrepeat

; .A = logical IRQ#.  Save the caller's ROM page and switch to page 0 for the dispatcher.
            CLABEL      IRQ_ENTRY
            phx
            ldx         W_REGISTER
            stz         W_REGISTER                          ; Now on page 0 (this same code)
            jmp         IRQ_DISPATCH

; .A = ROM page to return to.  Stack: .X, .A, then the interrupt frame.
            CLABEL      IRQ_EXIT
            sta         W_REGISTER                          ; Back on the interrupted page (this same code)
            plx
            pla
            rti

            CLABEL      NMI_ENTRY
            pha
            lda         W_REGISTER
            pha
            stz         W_REGISTER                          ; Now on page 0 (this same code)
            jsr         NMI_HANDLER
            pla
            sta         W_REGISTER                          ; Back on the interrupted page (this same code)
            pla
            rti

; Far call: ZP_FAR_A = .A, ZP_FAR_VEC = routine, ZP_FAR_PAGE = its ROM page.  Use FAR_GATE.
; .A, .X, .Y, C and V pass through in both directions; N/Z on return reflect .A.
; Not for use from IRQ handlers.
            CLABEL      FAR_CALL_A
            lda         W_REGISTER
            pha                                             ; Caller's page
            lda         ZP_FAR_PAGE
            sta         W_REGISTER                          ; Now on the far page (this same code)
            lda         ZP_FAR_A
            jsr         FAR_JMP_VEC
            sta         ZP_FAR_A
            pla
            sta         W_REGISTER                          ; Back on the caller's page (this same code)
            lda         ZP_FAR_A
            rts

            CLABEL      FAR_JMP_VEC
            jmp         (ZP_FAR_VEC)

; .A = byte at (ZP_D_XAM), with ROM page ZP_D_PAGE selected (only $E000-$FDFF is paged).
; For the disassembler, which runs on page 1 but usually examines the BIOS (page 0).
; Preserves .X, .Y, C; N/Z reflect .A
            CLABEL      PEEK_D_XAM
            phx
            ldx         W_REGISTER
            lda         ZP_D_PAGE
            sta         W_REGISTER                          ; Now on page ZP_D_PAGE (this same code)
            lda         (ZP_D_XAM)
            stx         W_REGISTER                          ; Back on the caller's page (this same code)
            plx
            ora         #0
            rts

; Far jump (no return): ZP_FAR_VEC = destination, ZP_FAR_PAGE = its ROM page.  Use FAR_JMP_GATE.
            CLABEL      FAR_JUMP
            lda         ZP_FAR_PAGE
            sta         W_REGISTER                          ; Now on the far page (this same code)
            jmp         (ZP_FAR_VEC)

.endmacro

.segment "COMMON_P0"
            COMMON_BLOCK
common_define .set 0
.segment "COMMON_P1"
            COMMON_BLOCK
.segment "COMMON_P2"
            COMMON_BLOCK
.segment "COMMON_P3"
            COMMON_BLOCK
.segment "COMMON_P4"
            COMMON_BLOCK
.segment "COMMON_P5"
            COMMON_BLOCK
.segment "COMMON_P6"
            COMMON_BLOCK
.segment "COMMON_P7"
            COMMON_BLOCK
.segment "COMMON_P8"
            COMMON_BLOCK
.segment "COMMON_P9"
            COMMON_BLOCK
.segment "COMMON_PA"
            COMMON_BLOCK
.segment "COMMON_PB"
            COMMON_BLOCK
.segment "COMMON_PC"
            COMMON_BLOCK
.segment "COMMON_PD"
            COMMON_BLOCK
.segment "COMMON_PE"
            COMMON_BLOCK
.segment "COMMON_PF"
            COMMON_BLOCK

; ****************************************************************************
; Gates: a local label that calls (or jumps to) a routine on another ROM page.
; .A, .X, .Y and C pass through to the routine and back (see FAR_CALL_A).

.macro FAR_GATE     name, target, page
name:
            sta         ZP_FAR_A
            lda         #<(target)
            sta         ZP_FAR_VEC
            lda         #>(target)
            sta         ZP_FAR_VEC + 1
            lda         #page
            sta         ZP_FAR_PAGE
            jmp         FAR_CALL_A
.endmacro

; Gate into a driver task: runs target in task (via TASK_CALL, see tasks.s).
; .A, .X, .Y and C pass through to the routine and back.
.macro TASK_GATE    name, target, task
name:
            pha
            LOAD_ADDR   target, ZP_TC_VEC
            lda         #task
            sta         ZP_TC_TASK
            pla
            jmp         TASK_CALL
.endmacro

.macro FAR_JMP_GATE name, target, page
name:
            lda         #<(target)
            sta         ZP_FAR_VEC
            lda         #>(target)
            sta         ZP_FAR_VEC + 1
            lda         #page
            sta         ZP_FAR_PAGE
            jmp         FAR_JUMP
.endmacro
