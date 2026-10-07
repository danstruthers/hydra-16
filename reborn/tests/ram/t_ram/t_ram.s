; ****************************************************************************
; t_ram - a RAM program (sdk/asm/hyx2.cfg: loaded at $0800 by SPAWN's loader, phase 4.1), started by t_load from a
; card with the arguments "a b c" and an fd map: its fds 0-2 closed (its lines go out on the bring-up console), its
; fd 3 a pipe's write end.  It checks what it was given: its arguments, its data as linked, its BSS cleared (the
; power-up's RAM is random), its header at its load address, its break at its top, its name, its module bank
; (none), the loader's fd closed, its fds the map's; then writes "hi" into fd 3, and ends with its failures as its
; code.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_ram", main

BSS_TEST        = 512                                       ; (Whole pages)

.zeropage
args:       .res        2
ptr:        .res        2
acc:        .res        1

.data
seed:       .byte       $5A, $A5                            ; (Its data: in its image, where it runs)

.bss
zeros:      .res        BSS_TEST                            ; (Cleared as it loads)
rec:        .res        SR_SIZE
info:       .res        TI_SIZE

.code
main:
            MOVR        args, r0
            stz         T_FAILS
            ldy         #0                                  ; Its arguments
            stz         acc
:
            lda         (args),Y
            eor         s_args,Y
            ora         acc
            sta         acc
            lda         s_args,Y
            beq         :+
            iny
            bra         :-
:
            lda         acc
            EXPECT_A    0, "t_ram: its arguments, a b c"
            lda         seed
            EXPECT_A    $5A, "t_ram: its data, as linked"
            lda         seed + 1
            EXPECT_A    $A5, "t_ram: and its second byte"
            LDR         ptr, zeros                          ; Its BSS
            ldx         #>BSS_TEST
            ldy         #0
            stz         acc
@page:
            lda         (ptr),Y
            ora         acc
            sta         acc
            iny
            bne         @page
            inc         ptr + 1
            dex
            bne         @page
            lda         acc
            EXPECT_A    0, "t_ram: its BSS cleared (512 bytes of it)"
            lda         HYX2_LOAD + HX_MAGIC
            EXPECT_A    'H', "t_ram: its header at its load address"
            lda         HYX2_LOAD + HX_FLAGS
            EXPECT_A    0, "t_ram: not in place"
            stz         r0                                  ; Its break: its top
            stz         r0 + 1
            jsr         BREAK
            MOVR        ptr, r0
            lda         ptr
            EXPECT_A    <__RAM_LAST__, "t_ram: its break at its top (low byte)"
            lda         ptr + 1
            EXPECT_A    >__RAM_LAST__, "t_ram: its break at its top (high byte)"
            jsr         GETPID
            pha
            LDR         r0, info
            pla
            jsr         TASKINFO
            lda         info + TI_NAME + 2
            EXPECT_A    'r', "t_ram: its name (t_ram), its header's"
            lda         info + TI_BANK
            EXPECT_A    $FF, "t_ram: no module bank"
            lda         info + TI_TYPE
            EXPECT_A    HT_PROGRAM, "t_ram: a program"
            LDR         r0, rec
            lda         #15
            jsr         FSTAT
            EXPECT_ERR  E_BADF, "t_ram: fd 15, the loader's, closed"
            LDR         r0, rec
            lda         #0
            jsr         FSTAT
            EXPECT_ERR  E_BADF, "t_ram: fd 0 closed, as the map has it"
            LDR         r0, s_hi
            LDR         r1, 2
            lda         #3
            jsr         WRITE
            EXPECT_A    2, "t_ram: hi, into fd 3 (the map's: t_load's pipe)"
            DONE        "t_ram"

.rodata
s_args:     .byte       "a b c", 0
s_hi:       .byte       "hi"
