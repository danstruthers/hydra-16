; ****************************************************************************
; as [-bl] file.s [out] - the assembler (phase 9): a program for the W65C02S from ca65's language, as the SDK's
; sources are written (sdk/asm), into a RAM program (HYX2: SPAWN reads it at $0800) at out (else file.s's name
; without its .s).  -l: a labels file too, out.lbl (ld65's -Ln form: db's l and dis's -l read it); -b: the bytes
; alone (no header), from .org's address ($0800 if there's none).
;   The assembler is the asm library's (modules/asm: its entry FILE, spec/asm.def), which BASIC's ASM blocks use too;
; this is its command: the arguments, the output's name, the library found (MODINFO), the call, the status.  The
; library takes this task's RAM from ASM_RAM to ASM_RAM_END while it runs: this program's own is below it.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"
.include "asmlib.inc"

            HYX2_PROGRAM "as", main

FNAME_LEN       = 64            ; A file's name, at most (PATH_MAX + 1)

.bss
asm_mod:    .res        1                                   ; The asm library's bank
flags:      .res        1                                   ; -b, -l (AF_RAW, AF_LABELS)
srcname:    .res        FNAME_LEN                           ; file.s
outname:    .res        FNAME_LEN                           ; out
minfo:      .res        ME_SIZE                             ; (MODINFO's, and its entry)
midx:       .res        1

.code

main:
            jsr         tl_start
            sta         flags                               ; (b: bit 0, AF_RAW; l: bit 1, AF_LABELS)
            lda         (tl_arg)                            ; file.s
            bne         :+
            jmp         tl_badusage
:
            LDR         r1, srcname
            jsr         argcpy
            jsr         tl_next                             ; [out]
            beq         @derive
            LDR         r1, outname
            jsr         argcpy
            jsr         tl_next
            beq         @go
            jmp         tl_badusage
@derive:
            ldy         #0                                  ; out: file.s less its .s
:
            lda         srcname,Y
            sta         outname,Y
            beq         :+
            iny
            bra         :-
:
            cpy         #3
            bcc         @noname
            lda         outname - 1,Y
            ora         #$20
            cmp         #'s'
            bne         @noname
            lda         outname - 2,Y
            cmp         #'.'
            bne         @noname
            lda         #0
            sta         outname - 2,Y
@go:
            jsr         find
            bcs         @nolib
            LDR         r0, srcname
            LDR         r1, outname
            lda         flags
            ASMCALL     ASM_FILE
            sta         tl_code
            jmp         tl_end
@noname:
            LDR         r0, s_noname
            bra         :+
@nolib:
            LDR         r0, s_nolib
:
            jsr         tl_warn
            jmp         tl_end

; The argument at tl_arg into the buffer at r1 (FNAME_LEN bytes at most, its 0 too)
argcpy:
            ldy         #0
:
            lda         (tl_arg),Y
            sta         (r1),Y
            beq         :+
            iny
            cpy         #FNAME_LEN - 1
            bne         :-
            lda         #0
            sta         (r1),Y
:
            rts

; asm_mod = the asm library's bank (the module "asm").  C = 1: there's none
find:
            stz         midx
@entry:
            LDR         r0, minfo
            lda         midx
            jsr         MODINFO
            bcs         @rts                                ; (Past the last)
            inc         midx
            lda         minfo + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @entry
            ldx         #0
:
            lda         s_asm,X
            cmp         minfo + ME_NAME,X
            bne         @entry
            inx
            cmp         #0
            bne         :-
            lda         minfo + ME_BANK
            sta         asm_mod
            clc
@rts:
            rts

.rodata
tl_name:    .byte       "as", 0
tl_flagset: .byte       "bl", 0
tl_usage:   .byte       "as [-bl] file.s [out]", 0
s_noname:   .byte       "file.s's name doesn't end .s: give the output's (as file.s out)", 0
s_nolib:    .byte       "no asm library", 0
s_asm:      .byte       "asm", 0

.include "toollib.s"
