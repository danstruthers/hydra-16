; ****************************************************************************
; dis.s - asm.h's dis_insn: the asm library (spec/asm.def) found at the first call (MODINFO: the module "asm"), then
; its DIS through XCALL, the struct dis read and written.  cc65's call: the struct's address in .A/.X.

            .export     _dis_insn

            .include    "zeropage.inc"
            .include    "hydra.inc"
            .include    "asmlib.inc"

DS_AT           = 0             ; struct dis: at, bytes, name, flags, len, kind, addr, text
DS_BYTES        = 2
DS_NAME         = 4
DS_FLAGS        = 6
DS_LEN          = 7
DS_KIND         = 8
DS_ADDR         = 9
DS_TEXT         = 11

            .bss
asm_mod:    .res        1                                   ; The library's bank (0: not found yet)
info:       .res        ME_SIZE                             ; (MODINFO's, and its entry)
idx:        .res        1

            .code

_dis_insn:
            sta         ptr1
            stx         ptr1 + 1
            lda         asm_mod
            bne         @have
            jsr         find
            bcc         @have
            lda         #0
            tax
            rts
@have:
            ldy         #DS_AT + 1                          ; (r1: its address; r0: its bytes; r3: the name)
            lda         (ptr1),y
            sta         r1 + 1
            dey
            lda         (ptr1),y
            sta         r1
            ldy         #DS_BYTES + 1
            lda         (ptr1),y
            sta         r0 + 1
            dey
            lda         (ptr1),y
            sta         r0
            ldy         #DS_NAME + 1
            lda         (ptr1),y
            sta         r3 + 1
            dey
            lda         (ptr1),y
            sta         r3
            clc                                             ; (r2: the struct's text)
            lda         ptr1
            adc         #DS_TEXT
            sta         r2
            lda         ptr1 + 1
            adc         #0
            sta         r2 + 1
            ldy         #DS_FLAGS
            lda         (ptr1),y
            ASMCALL     ASM_DIS
            ldy         #DS_LEN
            sta         (ptr1),y
            pha
            iny
            txa
            sta         (ptr1),y
            iny
            lda         r4
            sta         (ptr1),y
            iny
            lda         r4 + 1
            sta         (ptr1),y
            pla
            ldx         #0
            rts

; asm_mod = the asm library's bank.  OUT: C = 1 if there's none
find:
            stz         idx
@entry:
            lda         #<info
            sta         r0
            lda         #>info
            sta         r0 + 1
            lda         idx
            jsr         MODINFO
            bcs         @rts                                ; (Past the last)
            inc         idx
            lda         info + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @entry
            ldx         #0
:
            lda         s_asm,x
            cmp         info + ME_NAME,x
            bne         @entry
            inx
            cmp         #0
            bne         :-
            lda         info + ME_BANK
            sta         asm_mod
            clc
@rts:
            rts

            .rodata
s_asm:      .byte       "asm", 0
