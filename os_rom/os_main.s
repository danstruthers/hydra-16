.debuginfo

.macro W_SAVE_AND_RESET
            pha
            lda     W_REGISTER
            sta     ZP_W_SAVE
            stz     W_REGISTER
            pla
.endmacro

.macro W_RESTORE
            pha
            lda     ZP_W_SAVE
            sta     W_REGISTER
            pla
.endmacro

.segment "BIOS_P0"
.org RESET_ENTRY
            W_SAVE_AND_RESET                            ; Effectively a NOP, since we wouldn't be here if it was non-zero
                                                        ;   but kept for consistency across all OS ROM pages
            stz     T_REGISTER                          ; Make sure task 0 is selected
            stz     $00                                 ; Init RAM Bank selector
            stz     $01                                 ; Init ROM Bank selector
            ldx     #$FF                                ; Init stack pointer
            txs
            ; MOV     ZP_READ_PTR, ZP_WRITE_PTR                 ; remove when tasks_init is used

            jsr     IRQ_VECTOR_INIT
            jsr     TASKS_INIT                          ; Must be called before SERIAL_INIT
            jsr     SERIAL_INIT
            ;jsr     MMU_INIT
            ;jsr     SPI_INIT
            ;jsr     SPI_TEST
            jsr     SOUND_INIT
            jsr     SOUND_TEST
            jsr     DO_WELCOME
            jmp     SHELL_MAIN

DO_WELCOME:
            phx
            jsr     CLEAR_SCR
            ldx     0
            lda     HYDRA_WELCOME, X
            tay
@write_loop:
            inx
            lda     HYDRA_WELCOME, X
            jsr     WRITE_CHAR
            dey
            bne     @write_loop
            plx
            PRINT_CRLF
            ldx     #0

@vector_loop:
            stx         V_REGISTER
            phx
            PRINT_HEX   V_REGISTER
            PRINT_CHAR  #ASCII_BACKSPACE
            lda         V_REGISTER
            PRINT_HEX
            PRINT_CHAR  #ASCII_COLON
            PRINT_BYTE  $FFFF
            PRINT_BYTE  $FFFE
            PRINT_CHAR  #ASCII_SPACE
            plx
            inx
            cpx         #$10
            bcc         @vector_loop
            PRINT_CRLF_JMP

SPI_TEST:
            ldx         #SPI_DEV_0
            jsr         SPI_INIT_DELAY
            SPI_SEND_CMD 0,    0, 0, 0,   0, $4A        ; CMD0
            SPI_SEND_CMD 8,    0, 0, 1, $AA, $43        ; CMD8
@loop:
            SPI_SEND_CMD 58,   0, 0, 0,   0             ; CMD58
            SPI_SEND_CMD 41, $40, 0, 0,   0             ; ACMD41
            bne         @loop
            rts

; A: S/W interrupt number
SW_INT:
            asl                                         ; move int# to V[4..7]
            asl
            asl
            asl
            ora         #$F
            sta         V_REGISTER
            brk                                         ; force an interrupt
            rts

.segment "BIOS_P1"
            W_SAVE_AND_RESET

.segment "BIOS_P2"
            W_SAVE_AND_RESET

.segment "BIOS_P3"
            W_SAVE_AND_RESET
            
.segment "BIOS_P4"
            W_SAVE_AND_RESET

.segment "BIOS_P5"
            W_SAVE_AND_RESET

.segment "BIOS_P6"
            W_SAVE_AND_RESET

.segment "BIOS_P7"
            W_SAVE_AND_RESET

.segment "BIOS_P8"
            W_SAVE_AND_RESET

.segment "BIOS_P9"
            W_SAVE_AND_RESET

.segment "BIOS_PA"
            W_SAVE_AND_RESET

.segment "BIOS_PB"
            W_SAVE_AND_RESET

.segment "BIOS_PC"
            W_SAVE_AND_RESET

.segment "BIOS_PD"
            W_SAVE_AND_RESET

.segment "BIOS_PE"
            W_SAVE_AND_RESET

.segment "BIOS_PF"
            W_SAVE_AND_RESET

