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
.if I < 15
            jmp         IRQ_ENTRY
.endif                                                      ; (The last one runs on into it)
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

; The fast interrupt handlers (IRQ_INIT points the VIA's, the ACIA's and the YM2151's vectors here, not at
; their IRQ stubs): on to page 2, IRQ_FAST_P2 (serfast.s), with no dispatcher.  .Y = which (0 VIA, 1 ACIA,
; 2 YM2151), .X = the interrupted page; the interrupted .A, .X and .Y are on the stack.
            CLABEL      VIA_IRQ_STUB
            pha
            lda         #0
            bra         :+
            CLABEL      YM_IRQ_STUB
            pha
            lda         #2
            bra         :+
            CLABEL      SER_IRQ_STUB
            pha
            lda         #1
:
            phx
            phy
            ldx         W_REGISTER
            tay
            lda         #2
            sta         W_REGISTER                          ; Now on page 2 (this same code)
            jmp         IRQ_FAST_P2

; From a fast handler, for what it leaves to the dispatcher (the drivers' handlers, or a task switch):
; .A = the logical IRQ# (or IRQ_TICK), .X = the interrupted page, the interrupted .A and .X on the stack,
; as an IRQ stub leaves them
            CLABEL      IRQ_FAST_SLOW
            stz         W_REGISTER                          ; Now on page 0 (this same code)
            jmp         IRQ_DISPATCH

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

; Compact far call: `jsr FAR_INLINE` followed by `.word routine` and `.byte page` (FAR_GATE_INLINE).
; Reads the inline data (on the caller's page), then continues as FAR_CALL_A (it runs on into it).
            CLABEL      FAR_INLINE
            sta         ZP_FAR_A
            pla                                             ; Address of the inline data - 1
            sta         ZP_FAR_VEC
            pla
            sta         ZP_FAR_VEC + 1
            phy
            ldy         #3
            lda         (ZP_FAR_VEC),Y                      ; Page
            sta         ZP_FAR_PAGE
            dey
            lda         (ZP_FAR_VEC),Y                      ; Routine, high
            pha
            dey
            lda         (ZP_FAR_VEC),Y                      ; Routine, low
            sta         ZP_FAR_VEC
            pla
            sta         ZP_FAR_VEC + 1
            ply                                             ; (Returns to the gate's caller)

; Far call: ZP_FAR_A = .A, ZP_FAR_VEC = routine, ZP_FAR_PAGE = its ROM page.  Use FAR_GATE_INLINE.
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

; .A = the byte at (ZP_FP),Y on BIOS ROM page .A (only $E000-$FDFF is paged): for FP_BIOS far pointers (fp.s),
; and the disassembler's and WOZMON's reads (PEEK_D_XAM, on their pages).  Preserves .X, .Y, C
            CLABEL      PEEK_PAGE
            phx
            ldx         W_REGISTER
            sta         W_REGISTER                          ; Now on that page (this same code)
            lda         (ZP_FP),Y
            stx         W_REGISTER                          ; Back on the caller's page (this same code)
            plx
            rts

; Far jump (no return): ZP_FAR_VEC = destination, ZP_FAR_PAGE = its ROM page (a task's start: tasks.s)
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

; A far call: 6 bytes (`jsr FAR_INLINE`, then the routine and its page, see FAR_INLINE).  .A, .X, .Y, C
; and V pass through; N/Z on return reflect .A.
.macro FAR_GATE_INLINE  name, target, page
name:
            jsr         FAR_INLINE
            .word       target
            .byte       page
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

; A gate to code on another page that never comes back (WOZMON, a task's end): 6 bytes, a far call whose return
; is never used (it leaves 3 bytes on the stack: the caller's page and FAR_CALL_A's return)
.macro FAR_JMP_GATE name, target, page
            FAR_GATE_INLINE name, target, page
.endmacro
