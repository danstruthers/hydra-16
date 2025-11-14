.debuginfo
.segment "MMU"

MMU_INIT:
            rts

MEM_TEST:
            PUSH_AX
            PRINT_CRLF
            stz         RAM_BANK_REG
            stz         T_REGISTER

@task_num_loop:
            lda         #$02
            ldx         #$7D                ; exclude the task serial buffers @ $7E00 && $7F00
            jsr         TEST_PAGE_RANGE
            ldx         #$A0                ; end of banked RAM

@ram_bank_loop:
            PRINT_BYTE  RAM_BANK_REG
            PRINT_CHAR  #ASCII_PERIOD
            lda         #$80
            jsr         TEST_PAGE_RANGE
            inc         RAM_BANK_REG
            lda         #NUM_RAM_BANKS
            cmp         RAM_BANK_REG
            bne         @ram_bank_loop
            ;inc        T_REGISTER
            ;lda        #$10
            ;cmp        T_REGISTER
            ;bne         @task_num_loop
            stz         U_REGISTER

@shared_banks_loop:
            lda         #$F0
            sta         RAM_BANK_REG

@shared_bank_loop:
            PRINT_BYTE  U_REGISTER
            PRINT_CHAR  #ASCII_PERIOD
            PRINT_BYTE  RAM_BANK_REG
            PRINT_CHAR  #ASCII_PERIOD
            lda         #$80
            jsr         TEST_PAGE_RANGE
            inc         RAM_BANK_REG        ; increment shared bank
            bne         @shared_bank_loop
            inc         U_REGISTER
            lda         U_REGISTER
            cmp         #$10
            bcc         @shared_banks_loop
            PULL_XA
            rts

; Pass HOB of first page to test in A, HOB of last page + 1 in X
TEST_PAGE_RANGE:
            sta         ZP_TEMP_VEC_H
            PRINT_BYTE
            PRINT_BYTE  #0
            PRINT_CHAR  #ASCII_PERIOD
            txa
            clc
            sbc         #0
            PRINT_BYTE
            PRINT_BYTE  #$FF
            PRINT_CHAR  #ASCII_COLON
            PRINT_CRLF
            stz         ZP_TEMP_VEC_L
            stz         ZP_TEMP

@loop_init:
            lda         #$EA                ; NOP test pattern

@loop:
            sta         (ZP_TEMP_VEC_L)
            cmp         (ZP_TEMP_VEC_L)
            beq         @next
            lda         #ASCII_BANG
            bra         @write

@next:
            inc         ZP_TEMP_VEC_L
            bne         @loop
            lda         #ASCII_PERIOD

@write:
            jsr         WRITE_CHAR 
            stz         ZP_TEMP
            inc         ZP_TEMP_VEC_H
            cpx         ZP_TEMP_VEC_H
            bne         @loop_init
            PRINT_CRLF_JMP
