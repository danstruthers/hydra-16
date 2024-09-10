.debuginfo
.segment "OS_MAIN"

MMU_INIT:
            rts

MEM_TEST:
            PUSH_AX
.ifpc02
            stz     RAM_BANK_REG
            stz     TASK_NUM_PORT
.else
            lda     #0
            sta     RAM_BANK_REG
            sta     TASK_NUM_PORT
.endif

@task_num_loop:
            lda     #$02
            ldx     #$7D                ; exclude the task serial buffers @ $7E00 && $7F00
            jsr     TEST_PAGE_RANGE
            ldx     #$A0                ; end of banked RAM

@ram_bank_loop:
            lda     RAM_BANK_REG
            jsr     WRITE_BYTE
            lda     #ASCII_PERIOD
            jsr     WRITECHAR
            lda     #$80
            jsr     TEST_PAGE_RANGE
            inc     RAM_BANK_REG
            lda     #NUM_RAM_BANKS
            cmp     RAM_BANK_REG
            bne     @ram_bank_loop
            ;inc     TASK_NUM_PORT
            ;lda     #$10
            ;cmp     TASK_NUM_PORT
            ;bne     @task_num_loop
            lda     #0

@shared_banks_loop:
            sta     TASK_NUM_PORT
            lda     #$F0
            sta     RAM_BANK_REG

@shared_bank_loop:
            lda     TASK_NUM_PORT
            jsr     WRITE_BYTE
            lda     #ASCII_PERIOD
            jsr     WRITECHAR
            lda     RAM_BANK_REG
            jsr     WRITE_BYTE
            lda     #ASCII_PERIOD
            jsr     WRITECHAR
            lda     #$80
            jsr     TEST_PAGE_RANGE
            inc     RAM_BANK_REG        ; increment shared bank
            bne     @shared_bank_loop
            clc
            lda     #$10
            adc     TASK_NUM_PORT       ; increment the shared bank-of-banks
            bcc     @shared_banks_loop
            PULL_XA
            rts

; Pass HOB of first page to test in A, HOB of last page + 1 in X
TEST_PAGE_RANGE:
            sta     ZP_TEMP_VEC_H
            jsr     WRITE_BYTE
            lda     #0
            jsr     WRITE_BYTE
            lda     #ASCII_PERIOD
            jsr     WRITECHAR
            txa
            clc
            sbc     #0
            jsr     WRITE_BYTE
            lda     #$FF
            jsr     WRITE_BYTE
            lda     #ASCII_COLON
            jsr     WRITECHAR
            jsr     WRITE_CRLF
.ifpc02
            stz     ZP_TEMP_VEC_L
            stz     ZP_TEMP
.else
            lda     #0
            sta     ZP_TEMP_VEC_L
            sta     ZP_TEMP
.endif
@loop_init:
            lda     #$EA                ; NOP test pattern

@loop:
            sta     (ZP_TEMP_VEC_L)
            cmp     (ZP_TEMP_VEC_L)
            beq     @next
            inc     ZP_TEMP

@next:
            inc     ZP_TEMP_VEC_L
            bne     @loop
            ; test ZP_TEMP.  if > 0, then print !, otherwise print . (dot)
            lda     ZP_TEMP
            beq     @write_dot
            lda     #ASCII_BANG
            bne     @write

@write_dot:
            lda     #ASCII_PERIOD

@write:
            jsr     WRITECHAR
.ifpc02
            stz     ZP_TEMP
.else
            lda     #0
            sta     ZP_TEMP
.endif
            inc     ZP_TEMP_VEC_H
            cpx     ZP_TEMP_VEC_H
            bne     @loop_init
            jmp     WRITE_CRLF
