.debuginfo

; BIOS ROM page 4, included inside `.scope PAGE4` (see all.s): the power-on self test, called through a
; gate from the reset code (os_main.s), before anything else runs.  (On page 4 with the other self
; tests, and the paged RAM line tests it runs: post_ram.s, which has the hex printing too.)

.segment "TESTS_P4"

; ****************************************************************************
; Power-on self test.  Checks the memory mapping the task system depends on, using only polled serial
; output (no IRQs, no drivers) and task switches that don't touch the stack.  Prints e.g.:
;       POST ZP:T ST:T LO:T 7D:T SH:S P1:4C
;   ZP/ST/LO/7D: $0080 / $0180 / $0280 / $7D80 are per-Task (T) or common to all tasks (C)
;   SH: shared RAM bank $F0 (U = 0) is Shared between tasks (S) or not (X)
;   P1: byte at forth_main on ROM page 1 (4C expected: its jmp)
; and a second line of paged RAM line tests (hex masks of bad lines, all 0 when good):
;       RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000 ...
;   U:x      U lines U0-U3 (bit n = Un), tested on shared bank $F0
;   bb:x/dd/aaaa  bank bb at $8000-$9FFF: bank register lines 0-3 (bb ^ 1/2/4/8), data lines D0-D7 and
;            address lines A0-A12.  Tested: the first bank of each shared RAM chip ($F0, $F4, $F8, $FC, with
;            U = 0; a missing chip shows as bad lines) and of each installed task RAM module.
;   Destructive: run before anything is kept in paged RAM.
; A T typed while it runs (held down through the reset) starts the hardware test instead of the OS
; (hwtest/, in the paged ROM).
POST_ACIA_CMD   = ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLID | ACIA_CMD_BIT_RID    ; No IRQs

.macro _M_POST_TASK_TEST    addr, label
            ldx                 #label - POST_STRINGS
            jsr                 POST_PUTS
            lda                 #$A5
            stz                 T_REGISTER                          ; Task 0 (no stack use until back in task 0)
            sta                 addr
            ldx                 #1
            stx                 T_REGISTER                          ; Task 1
            lda                 #$5A
            sta                 addr
            stz                 T_REGISTER                          ; Task 0
            lda                 addr
            ldx                 #'T'
            cmp                 #$A5
            beq                 :+
            ldx                 #'C'
:
            txa
            jsr                 POST_PUTC
.endmacro

POST:
            php
            sei
            lda                 #$10 | SR_SELECT                    ; 8-N-1
            sta                 ACIA_R_CTRL
            lda                 #POST_ACIA_CMD
            sta                 ACIA_R_CMD
            _M_POST_TASK_TEST   $0080, POST_S_ZP
            _M_POST_TASK_TEST   $0180, POST_S_ST
            _M_POST_TASK_TEST   $0280, POST_S_LO
            _M_POST_TASK_TEST   $7D80, POST_S_7D

            ldx                 #POST_S_SH - POST_STRINGS
            jsr                 POST_PUTS
            stz                 U_REGISTER
            ldx                 #$F0
            stz                 T_REGISTER                          ; Task 0: shared bank $F0
            stx                 RAM_BANK_REG
            lda                 #$C3
            sta                 $8000
            lda                 #1
            sta                 T_REGISTER                          ; Task 1: shared bank $F0
            stx                 RAM_BANK_REG
            lda                 $8000
            stz                 RAM_BANK_REG
            stz                 T_REGISTER                          ; Task 0
            stz                 RAM_BANK_REG
            ldx                 #'S'
            cmp                 #$C3
            beq                 :+
            ldx                 #'X'
:
            txa
            jsr                 POST_PUTC

            ldx                 #POST_S_P1 - POST_STRINGS
            jsr                 POST_PUTS
            LOAD_ADDR           ::POST_P1_PROBE, ZP_D_XAM
            lda                 #1
            sta                 ZP_D_PAGE
            jsr                 PEEK_D_XAM                          ; Byte at forth_main on ROM page 1
            stz                 ZP_D_PAGE
            jsr                 POST_PUTBYTE

            jsr                 POST_RAM_TEST                       ; Paged RAM lines (page 4, post_ram.s)
            ldx                 #POST_S_CRLF - POST_STRINGS
            jsr                 POST_PUTS
            lda                 ACIA_R_STATUS                       ; A T typed during it: the hardware test
            and                 #ACIA_STATUS_BIT_RDRF               ;   (hold T down while pressing reset)
            beq                 POST_NO_HWT
            lda                 ACIA_R_DATA
            and                 #$DF
            cmp                 #'T'
            bne                 POST_NO_HWT
            _M_HWT_ENTER

POST_NO_HWT:
            plp
            rts

; Print the POST string at offset .X in POST_STRINGS.  Modifies: .A, .X, .Y
POST_PUTS:
            lda                 POST_STRINGS,X
            beq                 :+
            jsr                 POST_PUTC
            inx
            bra                 POST_PUTS
:
            rts

; Polled serial output (Rockwell 65C51: wait for TDRE, with a timeout; WDC 65C51: its TDRE always says
; empty, so wait a character's time after each byte).  Modifies: .Y
POST_PUTC:
            ldy                 #0
:
            pha
            lda                 ACIA_R_STATUS
            and                 #ACIA_STATUS_BIT_TDRE
            bne                 :+
            pla
            dey
            bne                 :-
            pha
:
            pla
            sta                 ACIA_R_DATA
.if ::SER_ACIA = ::SER_ACIA_WDC                                     ; Its TDRE always says empty: wait a
            phx                                                     ;   whole character's time
            ldx                 #(SER_CHAR_CYCLES + 1279) / 1280    ; (1280 cycles per .X)
            ldy                 #0
:
            dey
            bne                 :-
            dex
            bne                 :-
            plx
.else
            ldy                 #0                                  ; Short delay after each byte
:
            dey
            bne                 :-
.endif
            rts

POST_STRINGS:
POST_S_ZP:  .byte ASCII_CR, ASCII_LF, "POST ZP:", 0
POST_S_ST:  .byte " ST:", 0
POST_S_LO:  .byte " LO:", 0
POST_S_7D:  .byte " 7D:", 0
POST_S_SH:  .byte " SH:", 0
POST_S_P1:  .byte " P1:", 0
POST_S_CRLF: .byte ASCII_CR, ASCII_LF, 0
