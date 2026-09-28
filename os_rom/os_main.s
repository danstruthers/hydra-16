.debuginfo
.macro ZERO_W
            lda     #0
            sta     W_REGISTER
.endmacro

.segment "BIOS_P0"
RESET_VECTOR_START:
            ZERO_W
            cld                                                     ; We don't like decimal mode
            sta                 T_REGISTER                          ; Make sure task 0 is selected
            sta                 $00                                 ; Init RAM Bank selector
            sta                 $01                                 ; Init ROM Bank selector
            ldx                 #$FF                                ; Init stack pointer
            txs

            jsr                 POST                                ; Power-on self test (polled serial, no IRQs)
            jsr                 IRQ_INIT                            ; Must be first: IRQ tables and vectors
            jsr                 TASKS_INIT                          ; Must be called before the drivers and MMU_INIT
            jsr                 MMU_INIT
            jsr                 IO_INIT                             ; The IO layer's devices (/dev/null, /dev/zero)
            jsr                 VIA_INIT
            lda                 #<SERIAL_DRIVER                     ; Serial driver in its own (Resident) task;
            ldy                 #>SERIAL_DRIVER                     ;   first: it must be started before
            ldx                 #SERIAL_TASK_NUM                    ;   anything prints (DRV_BOOT too)
            jsr                 DRV_START
            jsr                 DO_WELCOME                          ; (It clears the screen: before DRV_BOOT's reports)
            lda                 #<SOUND_DRIVER                      ; Sound driver in its own (Resident) task
            ldy                 #>SOUND_DRIVER
            ldx                 #SOUND_TASK_NUM
            jsr                 DRV_BOOT
            lda                 #<PIPE_DRIVER                       ; Pipe server in its own (Resident) task
            ldy                 #>PIPE_DRIVER
            ldx                 #PIPE_TASK_NUM
            jsr                 DRV_BOOT
            lda                 #<STORAGE_DRIVER                    ; Storage (/dev/sd) in its own (Resident) task
            ldy                 #>STORAGE_DRIVER
            ldx                 #STORAGE_TASK_NUM
            jsr                 DRV_BOOT
            ;jsr                 SND_CALL_TEST

; Start the shell in its own task (the default serial-capture task), start the scheduler's tick, and
; hand the CPU over
            lda                 #<SHELL_MAIN
            ldy                 #>SHELL_MAIN
            ldx                 #SHELL_TASK_NUM
            jsr                 TASK_PREPARE
            jsr                 SCHED_START
            jsr                 YIELD

; The system task is the idle task: the scheduler only runs it when no other task can run
@idle:
            wai
            bra                 @idle

; ****************************************************************************
; Power-on self test.  Checks the memory mapping the task system depends on, using only polled serial
; output (no IRQs, no drivers) and task switches that don't touch the stack.  Prints e.g.:
;       POST ZP:T ST:T LO:T 7D:T SH:S P1:4C
;   ZP/ST/LO/7D: $0080 / $0180 / $0280 / $7D80 are per-Task (T) or common to all tasks (C)
;   SH: shared RAM bank $F0 (U = 0) is Shared between tasks (S) or not (X)
;   P1: byte at $EA00 on ROM page 1 (4C expected: the jmp at forth_main)
; and a second line of paged RAM line tests (hex masks of bad lines, all 0 when good):
;       RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000 ...
;   U:x      U lines U0-U3 (bit n = Un), tested on shared bank $F0
;   bb:x/dd/aaaa  bank bb at $8000-$9FFF: bank register lines 0-3 (bb ^ 1/2/4/8), data lines D0-D7 and
;            address lines A0-A12.  Tested: the first bank of each shared RAM chip ($F0, $F4, $F8, $FC, with
;            U = 0; a missing chip shows as bad lines) and of each installed task RAM module.
;   Destructive: run before anything is kept in paged RAM.
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
            LOAD_ADDR           $EA00, ZP_D_XAM
            lda                 #1
            sta                 ZP_D_PAGE
            jsr                 PEEK_D_XAM                          ; Byte at $EA00 on ROM page 1
            stz                 ZP_D_PAGE
            jsr                 POST_PUTBYTE

            jsr                 POST_RAM_TEST                       ; Paged RAM lines (page 2, post_ram.s)
            ldx                 #POST_S_CRLF - POST_STRINGS
            jsr                 POST_PUTS
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

; Modifies: .X, .Y
POST_PUTBYTE:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr                 POST_PUTHEX
            pla

POST_PUTHEX:
            and                 #$0F
            tax
            lda                 HEX_MAP,X

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
.if SER_ACIA = SER_ACIA_WDC                                         ; Its TDRE always says empty: wait a
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

; Start a driver at boot (DRV_START), and say so if its init fails: "<NAME> FAIL ee" (ee = the error).
; The serial driver must be running already.  IN: .A.Y = DriverInfo, .X = task
DRV_BOOT:
            pha
            phy
            jsr                 DRV_START
            bcc                 @done
            ply
            sty                 ZP_TEMP_VEC + 1                     ; The DriverInfo
            ply
            sty                 ZP_TEMP_VEC
            pha
            PRINT_CRLF
            ldy                 #DriverInfo::name + 1
            lda                 (ZP_TEMP_VEC),Y
            tax
            dey
            lda                 (ZP_TEMP_VEC),Y
            phx
            ply
            jsr                 WRITE_HSTRING                       ; Its name
            PRINT_CHAR          #' ', #'F', #'A', #'I', #'L', #' '
            pla
            PRINT_BYTE
            PRINT_CRLF
            rts

@done:
            ply
            pla
            rts

DO_WELCOME:
            jsr                 CLEAR_SCR
            _M_WRITE_HSTRING    HYDRA_WELCOME
            PRINT_CRLF_JMP

; A: S/W interrupt number
; Preserves .X and V
SW_INT:
            php                                                     ; No task switch while V is ours (BRK runs
            sei                                                     ;   even with IRQs off)
            phx
            ldx                 V_REGISTER                          ; Save V (shared pseudo-register)
            asl                                                     ; move int# to V[4..7]
            asl
            asl
            asl
            ora                 #IRQ_NUMBER_SW                      ; S/W IRQ vector in V[0..3]
            sta                 V_REGISTER
            brk                                                     ; force an interrupt
            .byte               $00                                 ; BRK signature byte (RTI returns past it)
            stx                 V_REGISTER                          ; Restore prior V
            plx
            plp
            rts

; Start of every other BIOS page: the RESET entry at $E000 zeroes W, and execution continues on page 0
; right after RESET_VECTOR_START's ZERO_W.  The rest of each page is filled with NOPs by the linker
; (fillval), except for the COMMON block and the vectors.
.macro OTHER_PAGE_FILLER
            ZERO_W
.endmacro

.segment "BIOS_P1"                                                  ; HyForth and the disassembler follow (page1.s)
            OTHER_PAGE_FILLER
.segment "BIOS_P2"
            OTHER_PAGE_FILLER
.segment "BIOS_P3"
            OTHER_PAGE_FILLER
.segment "BIOS_P4"
            OTHER_PAGE_FILLER
.segment "BIOS_P5"
            OTHER_PAGE_FILLER
.segment "BIOS_P6"
            OTHER_PAGE_FILLER
.segment "BIOS_P7"
            OTHER_PAGE_FILLER
.segment "BIOS_P8"
            OTHER_PAGE_FILLER
.segment "BIOS_P9"
            OTHER_PAGE_FILLER
.segment "BIOS_PA"
            OTHER_PAGE_FILLER
.segment "BIOS_PB"
            OTHER_PAGE_FILLER
.segment "BIOS_PC"
            OTHER_PAGE_FILLER
.segment "BIOS_PD"
            OTHER_PAGE_FILLER
.segment "BIOS_PE"
            OTHER_PAGE_FILLER
.segment "BIOS_PF"
            OTHER_PAGE_FILLER
