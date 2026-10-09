; ****************************************************************************
; t_xcall - XCALL, run as init with the library module t_lib: its bank found in the module directory (MODINFO), its
; routines called: registers and flags through both ways, r0-r13 too, its own bank set while it runs and this
; module's back after, a system call from it.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_xcall", main

LIB_ADD         = $A030
LIB_BANK        = $A033
LIB_SAY         = $A036

.zeropage
bank:       .res        1                                   ; t_lib's bank
mine:       .res        1                                   ; This module's
entry:      .res        1
res_a:      .res        1                                   ; What a call gave back
res_y:      .res        1
res_p:      .res        1
res_r0:     .res        2

.bss
me:         .res        ME_SIZE

.code
main:
            stz         T_FAILS
            lda         ROM_BANK
            sta         mine
            stz         entry                               ; t_lib, in the module directory
@find:
            LDR         r0, me
            lda         entry
            jsr         MODINFO
            bcs         @none
            ldx         #0
:
            lda         me + ME_NAME,X
            cmp         s_lib,X
            bne         @next
            inx
            cmp         #0
            bne         :-
            lda         me + ME_TYPE
            EXPECT_A    HT_LIBRARY, "t_lib: in the module directory, a library"
            lda         me + ME_BANK
            sta         bank
            bra         @found

@next:
            inc         entry
            bra         @find

@none:
            NOTOK       "t_lib: in the module directory"
            jmp         @done

@found:
            lda         bank                                ; ---- Registers and flags, both ways
            sta         r14
            LDR         r15, LIB_ADD
            LDR         r0, $1234
            LDR         r1, $1111
            lda         #3
            ldx         #4
            ldy         #$5A
            clc
            jsr         XCALL
            php                                             ; (What came back: the checks use .A, .X, .Y, r0)
            sta         res_a
            sty         res_y
            MOVR        res_r0, r0
            pla
            sta         res_p
            lda         res_y
            EXPECT_A    $5A, ".Y through to the routine and back"
            lda         res_p
            and         #$01
            EXPECT_A    1, "the routine's flags back (C = 1)"
            lda         res_a
            EXPECT_A    7, "XCALL: .A and .X in, .A out (3 + 4)"
            lda         res_r0 + 1
            EXPECT_A    $23, "r0 and r1 in, r0 out ($1234 + $1111: high byte)"
            lda         res_r0
            EXPECT_A    $45, "(low byte)"
            lda         bank                                ; ---- Banks
            sta         r14
            LDR         r15, LIB_BANK
            jsr         XCALL
            sta         res_a
            lda         ROM_BANK
            sta         res_y
            lda         res_a
            cmp         bank
            beq         :+
            NOTOK       "the routine runs in its own bank"
            bra         :++
:
            OK          "the routine runs in its own bank"
:
            lda         res_y
            cmp         mine
            beq         :+
            NOTOK       "and this module's is back after it"
            bra         :++
:
            OK          "and this module's is back after it"
:
            lda         bank                                ; ---- A system call from the library
            sta         r14
            LDR         r15, LIB_SAY
            jsr         XCALL
            EXPECT_OK   "a system call from a library routine (PUTS)"
@done:
            DONE        "t_xcall"

.rodata
s_lib:      .byte       "t_lib", 0
