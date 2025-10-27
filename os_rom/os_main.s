.debuginfo
.segment "OS_MAIN"

.include "defines.s"
OS_MAIN:
            lda     #0
            sta     T_REGISTER
            sta     $00                                 ; Init RAM Bank selector
            sta     $01                                 ; Init ROM Bank selector
            ldx     #$FF                                ; Init stack pointer
            txs
            MOV     ZP_READ_PTR, ZP_WRITE_PTR                 ; remove when tasks_init is used

            jsr     IRQ_VECTOR_INIT
            jsr     TASKS_INIT
            jsr     SERIAL_INIT
            jsr     MMU_INIT
            ;jsr     SPI_INIT
            ;jsr     SPI_TEST
            ;jsr     SOUND_INIT
            ;jsr     SOUND_TEST
            jsr     DO_WELCOME
            jsr     SHELL_MAIN
            brk                                         ; Halt and catch fire!

.include "bios.s"
.include "math.s"
.include "sound.s"
.include "wozmon.s"
.include "mmu.s"
.include "tasks.s"
.include "shell.s"

.segment "OS_MAIN"
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
            ldx     #SPI_DEV_0
            jsr     SPI_INIT_DELAY
            SPI_SEND_CMD 0,    0, 0, 0,   0, $4A        ; CMD0
            SPI_SEND_CMD 8,    0, 0, 1, $AA, $43        ; CMD8
@loop:
            SPI_SEND_CMD 58,   0, 0, 0,   0             ; CMD58
            SPI_SEND_CMD 41, $40, 0, 0,   0             ; ACMD41
            bne     @loop
            rts
