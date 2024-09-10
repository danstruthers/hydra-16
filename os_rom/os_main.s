.debuginfo
.segment "OS_MAIN"

.include "defines.s"
OS_MAIN:
            lda     #0
            sta     TASK_NUM_PORT
            sta     $00                                 ; Init RAM Bank selector
            sta     $01                                 ; Init ROM Bank selector
            ldx     #$FF                                ; Init stack pointer
            txs
            jsr     TASKS_INIT
            jsr     SERIAL_INIT
            jsr     MMU_INIT
            ;MOV     ZP_READ_PTR, ZP_WRITE_PTR                 ; remove when tasks_init is used
            ;jsr     SPI_INIT
            ;jsr     SPI_TEST
            jsr     SHELL_MAIN
            brk                                         ; Halt and catch fire!

.include "bios.s"
.include "wozmon.s"
.include "mmu.s"
.include "tasks.s"
.include "shell.s"

.segment "OS_MAIN"
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
