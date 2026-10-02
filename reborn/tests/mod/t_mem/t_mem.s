; ****************************************************************************
; t_mem - memory (phase 1.8), run as init with t_child and two RAM modules: BREAK, PAGES_ALLOC and PAGES_FREE,
; BANKS, BANKS_ALLOC and BANKS_FREE, and a shared segment seen by another task (and copied with kcopy), freed when
; the last task attached detaches or ends.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_mem", main

.zeropage
brk0:       .res        2                                   ; The break at the start
seg:        .res        1
seg_u:      .res        1
seg_bank:   .res        1

.bss
buf:        .res        64
args:       .res        4                                   ; "gNN" for t_child

.code
main:
            stz         T_FAILS

; ---- BREAK
            stz         r0
            stz         r0 + 1
            jsr         BREAK
            php
            MOVR        brk0, r0                            ; (Before the check: it prints, with r0)
            plp
            EXPECT_OK   "BREAK, to ask"
            lda         brk0 + 1
            EXPECT_A    >(__RAM_LAST__), "the break starts at the module's data and BSS (high byte)"
            lda         brk0
            EXPECT_A    <(__RAM_LAST__), "the break starts at the module's data and BSS (low byte)"
            LDR         r0, $3000
            jsr         BREAK
            EXPECT_OK   "BREAK up to $3000"
            LDR         r0, $0400
            jsr         BREAK
            EXPECT_ERR  E_INVAL, "BREAK below the program's own data: E_INVAL"
            LDR         r0, $8001
            jsr         BREAK
            EXPECT_ERR  E_NOMEM, "BREAK past the top of RAM: E_NOMEM"

; ---- Pages: from the top down
            lda         #2
            jsr         PAGES_ALLOC
            EXPECT_A    $7E, "PAGES_ALLOC 2: pages $7E-$7F"
            lda         #$C5                                ; (They're RAM)
            sta         $7FFF
            lda         $7FFF
            EXPECT_A    $C5, "and they're RAM"
            lda         #1
            jsr         PAGES_ALLOC
            EXPECT_A    $7D, "PAGES_ALLOC 1: page $7D, under them"
            LDR         r0, $7E00
            jsr         BREAK
            EXPECT_ERR  E_NOMEM, "BREAK over a page given out: E_NOMEM"
            LDR         r0, $7E00
            lda         #2
            jsr         PAGES_FREE
            EXPECT_OK   "PAGES_FREE 2 at $7E00"
            LDR         r0, $7E00
            lda         #2
            jsr         PAGES_FREE
            EXPECT_ERR  E_INVAL, "PAGES_FREE of pages not given: E_INVAL"
            lda         #3
            jsr         PAGES_ALLOC
            EXPECT_A    $7A, "PAGES_ALLOC 3: only 2 free above page $7D, so $7A-$7C"
            lda         #0
            jsr         PAGES_ALLOC
            EXPECT_ERR  E_INVAL, "PAGES_ALLOC 0: E_INVAL"
            LDR         r0, $7D00
            lda         #1
            jsr         PAGES_FREE
            EXPECT_OK   "PAGES_FREE page $7D"
            LDR         r0, $7A00
            lda         #3
            jsr         PAGES_FREE
            EXPECT_OK   "PAGES_FREE pages $7A-$7C"
            lda         #81                                 ; ($3000-$7FFF: 80 pages)
            jsr         PAGES_ALLOC
            EXPECT_ERR  E_NOMEM, "PAGES_ALLOC 81, with 80 above the break: E_NOMEM"
            lda         #80
            jsr         PAGES_ALLOC
            EXPECT_A    $30, "PAGES_ALLOC 80: every page above the break"
            LDR         r0, $3000
            lda         #80
            jsr         PAGES_FREE
            EXPECT_OK   "and back"

; ---- Banks
            jsr         BANKS
            ldx         r0                                  ; (Before the check: it prints, with r0)
            stx         seg
            EXPECT_A    $20, "BANKS: 32 (two RAM modules)"
            lda         seg
            EXPECT_A    $03, "BANKS: modules 0 and 1"
            lda         #3
            jsr         BANKS_ALLOC
            EXPECT_A    0, "BANKS_ALLOC 3: banks 0-2"
            lda         #1
            jsr         BANKS_ALLOC
            EXPECT_A    3, "BANKS_ALLOC 1: bank 3"
            lda         #0
            ldx         #3
            jsr         BANKS_FREE
            EXPECT_OK   "BANKS_FREE 0-2"
            lda         #0
            ldx         #3
            jsr         BANKS_FREE
            EXPECT_ERR  E_INVAL, "BANKS_FREE of banks not given: E_INVAL"
            lda         #29
            jsr         BANKS_ALLOC
            EXPECT_ERR  E_NOMEM, "BANKS_ALLOC 29: banks 4-32, but 32 is no module's: E_NOMEM"
            lda         #28
            jsr         BANKS_ALLOC
            EXPECT_A    4, "BANKS_ALLOC 28: banks 4-31, the rest of modules 0 and 1"
            lda         #4
            jsr         BANKS_ALLOC
            EXPECT_ERR  E_NOMEM, "BANKS_ALLOC 4 with banks 0-2 left: E_NOMEM"
            lda         #3
            jsr         BANKS_ALLOC
            EXPECT_A    0, "BANKS_ALLOC 3: banks 0-2"

; ---- A shared segment, seen by another task
            lda         #2
            jsr         SEG_CREATE
            sta         seg
            EXPECT_OK   "SEG_CREATE 2 banks"
            lda         seg
            ldx         #1
            jsr         SEG_MAP
            EXPECT_OK   "SEG_MAP: its second bank"
            lda         seg
            ldx         #2
            jsr         SEG_MAP
            EXPECT_ERR  E_RANGE, "SEG_MAP: a bank past its end, E_RANGE"
            lda         seg
            ldx         #0
            jsr         SEG_MAP
            sta         seg_u
            stx         seg_bank
            sta         U_REGISTER                          ; Its first bank: $A7 at $8000
            stx         RAM_BANK
            lda         #$A7
            sta         BANK_WINDOW
            lda         #'g'                                ; t_child "g" + the segment, in hex: attach, read
            sta         args
            lda         seg
            jsr         hexbyte
            stz         args + 3
            LDR         r0, s_child
            LDR         r1, args
            lda         #0
            jsr         SPAWN
            stz         r0
            stz         r0 + 1
            lda         #$FF
            jsr         WAIT
            txa
            EXPECT_A    $A7, "another task attached to it reads what this one wrote"

; ---- kcopy with a buffer in the shared bank: to the kernel task, and back to here
            LDR         r0, BANK_WINDOW
            LDR         r1, $4000
            LDR         r2, 64
            lda         #0
            clc
            jsr         DBG_KCOPY
            LDR         r0, buf
            LDR         r1, $4000
            LDR         r2, 64
            lda         #0
            sec
            jsr         DBG_KCOPY
            lda         buf
            EXPECT_A    $A7, "kcopy from a shared bank"
            stz         RAM_BANK
            stz         U_REGISTER

            lda         seg
            jsr         SEG_DETACH
            EXPECT_OK   "SEG_DETACH (the child's end detached it too)"
            lda         seg
            jsr         SEG_ATTACH
            EXPECT_ERR  E_INVAL, "and with nobody attached it's gone: SEG_ATTACH, E_INVAL"
            lda         #0
            jsr         SEG_CREATE
            EXPECT_ERR  E_INVAL, "SEG_CREATE 0: E_INVAL"
            lda         #129
            jsr         SEG_CREATE
            EXPECT_ERR  E_NOMEM, "SEG_CREATE 129: E_NOMEM"

            DONE        "t_mem"

; .A as two hex digits at args + 1
hexbyte:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         @digit
            sta         args + 1
            pla
            jsr         @digit
            sta         args + 2
            rts

@digit:
            and         #$0F
            cmp         #10
            bcc         :+
            adc         #'a' - '0' - 10 - 1                 ; (C = 1)
:
            adc         #'0'
            rts

.rodata
s_child:    .byte       "#m/t_child", 0
