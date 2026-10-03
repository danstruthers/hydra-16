; ****************************************************************************
; t_scall - calls into another task (phase 1, spike S3), run as init, with t_drv (task E) and t_child: a driver's
; serve entry run in its task, results and errors back, the caller seen, the errors of a call to no driver, a
; busy driver making its other callers wait, and the round trip's time (the marks "<scall" and "scall>" around
; 1000 calls: sim/test.js divides).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_scall", main

DRV             = $0E                                       ; t_drv: the boot driver after kdev, so task E
CALLS           = 1000

.zeropage
n:          .res        2
t0:         .res        1
calls0:     .res        1
fake:       .res        2                                   ; (The baseline's count of calls)

.code
main:
            stz         T_FAILS
            lda         #5
            ldy         #DRV
            jsr         DBG_SCALL
            EXPECT_A    $5A, "the driver's init ran, in its task (task E)"
            lda         #0
            ldx         #$41
            ldy         #DRV
            jsr         DBG_SCALL
            EXPECT_A    $42, "a call's result: .X + 1"
            lda         #1
            ldy         #DRV
            jsr         DBG_SCALL
            EXPECT_A    1, "the driver sees its caller in .Y"
            lda         #2
            ldy         #DRV
            jsr         DBG_SCALL
            EXPECT_ERR  E_INVAL, "a call's error comes back"
            lda         #9
            ldy         #DRV
            jsr         DBG_SCALL
            EXPECT_ERR  E_NOSYS, "and another"

; ---- Calls to no driver
            lda         #0
            ldy         #16
            jsr         DBG_SCALL
            EXPECT_ERR  E_SRCH, "a call to task 16: E_SRCH"
            lda         #0
            ldy         #5
            jsr         DBG_SCALL
            EXPECT_ERR  E_NODEV, "a call to a free task: E_NODEV"
            lda         #0
            ldy         #1
            jsr         DBG_SCALL
            EXPECT_ERR  E_NODEV, "a call to a program (this one): E_NODEV"
            LDR         r0, s_drv
            stz         r1
            stz         r1 + 1
            lda         #0
            jsr         SPAWN
            EXPECT_ERR  E_NOEXEC, "SPAWN of a driver: E_NOEXEC"

; ---- The round trip's time: 1000 calls, less the same loop calling the driver's code here ("<base" "base>")
            MARK        "<base"
            lda         #<CALLS
            sta         n
            lda         #>CALLS
            sta         n + 1
@base:
            lda         #0
            ldx         #0
            ldy         #DRV
            jsr         local
            lda         n
            bne         :+
            dec         n + 1
:
            dec         n
            lda         n
            ora         n + 1
            bne         @base
            MARK        "base>"
            MARK        "<scall"
            lda         #<CALLS
            sta         n
            lda         #>CALLS
            sta         n + 1
@call:
            lda         #0
            ldx         #0
            ldy         #DRV
            jsr         DBG_SCALL
            lda         n
            bne         :+
            dec         n + 1
:
            dec         n
            lda         n
            ora         n + 1
            bne         @call
            MARK        "scall>"
            jsr         t_crlf
            lda         #4
            ldy         #DRV
            jsr         DBG_SCALL
            sta         calls0
            OK          "1000 calls made"

; ---- A busy driver: two children each make a call that sleeps 10 ticks; the second waits for the first
            jsr         TICKS
            sta         t0
            LDR         r0, s_child
            LDR         r1, s_c0a
            lda         #0
            jsr         SPAWN
            LDR         r0, s_child
            LDR         r1, s_c0a
            lda         #0
            jsr         SPAWN
            stz         r0
            stz         r0 + 1
            lda         #$FF
            jsr         WAIT
            stz         r0
            stz         r0 + 1
            lda         #$FF
            jsr         WAIT
            jsr         TICKS
            sec
            sbc         t0
            cmp         #20
            bcs         :+
            NOTOK       "a busy driver: the second call waits for the first (20 ticks)"
            bra         @count
:
            OK          "a busy driver: the second call waits for the first (20 ticks)"
@count:
            lda         #4
            ldy         #DRV
            jsr         DBG_SCALL
            sec
            sbc         calls0
            EXPECT_A    3, "the driver served each call once"

            DONE        "t_scall"

; t_drv's serve entry for op 0, here (the baseline: the call's own cost is the difference).  Through a jmp, as
; DBG_SCALL is through the jump table
local:
            jmp         @serve

@serve:
            inc         fake
            bne         :+
            inc         fake + 1
:
            cmp         #0
            bne         :+
            inx
            txa
            clc
:
            rts

.rodata
s_drv:      .byte       "#m/t_drv", 0
s_child:    .byte       "#m/t_child", 0
s_c0a:      .byte       "c0a", 0
