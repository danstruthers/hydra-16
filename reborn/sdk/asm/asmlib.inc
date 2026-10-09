; asmlib.inc - the asm library's entries, constants and call macro, for programs in assembly.  Made by tools/apigen.js from spec/: don't edit
; A call (spec/asm.def): XCALL, r15 the entry (its address here), r14 the library's bank (MODINFO finds it, by
; the name "asm").  ASMCALL ASM_DIS makes one, .A, .X and .Y the entry's, r14 from a byte of the program's,
; asm_mod.

.macro ASMCALL     entry
            pha
            lda         #<(entry)
            sta         r15
            lda         #>(entry)
            sta         r15 + 1
            lda         asm_mod
            sta         r14
            pla
            jsr         XCALL
.endmacro

; ---- asm: The W65C02S's instructions, as as writes them: one disassembled; a source assembled
ASM_DIS         = $A030       ; r0 = the instruction's bytes (its opcode and the two after it); r1 = its address; r2 = the text's place (DIS_MAX bytes); r3 = a name for the address its operand names (0: none); .A = the flags (DF_)
                              ; -> .A = its length (1-3); .X = its kind (DK_); .Y = the text's length (a 0 after it); r4 = the address its operand names (a branch's: where it goes)
ASM_FILE        = $A033       ; r0 = the source's name; r1 = the output's; .A = the flags (AF_)
                              ; -> .A = the status (0, or 1 after an error); C = 1 if it's 1
ASM_BEGIN       = $A036       ; r0 = the origin (the image's first address); r1 = the source's name (its messages'); .A = the flags (AF_QUIET)
                              ; -> C = 1 if there's no room (RAM banks)
ASM_PASS        = $A039       ; .A = the pass (1, 2, 3)
ASM_DEFINE      = $A03C       ; r0 = a name (32 characters at most); r1/r2 = its value (32 bits)
                              ; -> C = 1 if there's no room, or the name's too long
ASM_LINE        = $A03F       ; r0 = a line (zero-terminated, 255 bytes at most); r1 = its number
                              ; -> C = 1 if it had an error
ASM_END         = $A042       ; -
                              ; -> C = 1 if the pass had an error
ASM_SYMBOL      = $A045       ; r0 = a name
                              ; -> C = 0 and r1/r2 = its value (32 bits); C = 1 if there's no such symbol
ASM_IMAGE       = $A048       ; -
                              ; -> .A = the image's first bank (its first 8K: the origin's bytes at $8000 in it); r0 = its length; r1 = its BSS's end (the first address past it)
ASM_DONE        = $A04B       ; .A = 1 to keep the image's first bank (the caller's from now on), else 0
ASM_ERROR       = $A04E       ; -
                              ; -> r0 = the first message since BEGIN (AF_QUIET's), zero-terminated ("" if there's none)

; ---- constants
ASM_RAM         = $5000       ; The assembler's state: the caller's RAM it takes while it runs, from here ...
ASM_RAM_END     = $7E00       ;   to here (not this)
AF_RAW          = $01         ; FILE's flags: the bytes alone from .org's address (as -b), not a RAM program
AF_LABELS       = $02         ;   a labels file too, the output's name and .lbl (as -l: ld65's -Ln form)
AF_QUIET        = $04         ; BEGIN's flags: messages kept for ERROR, not said
DIS_MAX         = 64          ; The room for DIS's text: an instruction as as writes it, its 0 too
DF_PAD          = $01         ; DIS's flags: its name padded to 12 columns (the SDK's sources' way), else one space
DK_ADDR         = $01         ; DIS's kinds: its operand is an address (r4: the zero page's or not), read or written
DK_GO           = $02         ;   it goes to the address r4 (a jump, jsr, a branch: bbr's and bbs's too)
DK_CALL         = $04         ;   a jsr (DK_GO too)
DK_END          = $08         ;   nothing after it is reached by going on from it (brk, rti, rts, stp, jmp, bra)
DK_UNDEF        = $10         ;   an opcode the W65C02S doesn't define (the NOP it runs: .byte and its bytes)
