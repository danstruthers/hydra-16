.debuginfo

.zeropage
ZP_M_BI_START:
            .res        2
ZP_M_SP1:
ZP_M_SP1_L:
            .res        1
ZP_M_SP1_H:
            .res        1
ZP_M_SP2:
ZP_M_SP2_L:
            .res        1
ZP_M_SP2_H:
            .res        1
ZP_M_SZ1:
            .res        1
ZP_M_TEMP:
            .res        1
ZP_M_TEMP2:
            .res        1
ZP_M_SV:
            .res        1

.segment "MMU"

.struct     Address
            l           .byte
            h           .byte
.endstruct

.struct     PageAddress
            page        .byte
            addr        .tag Address
.endstruct

; status byte => 
; Bit 0: Allocated (1=Allocated, 0=Free)        Is the memory free or allocated
; Bit 1: Paged (1=Paged, 0=Task)                Points to Paged RAM
; Bit 2: Shared (1=Shared, 0=Task)              Points to Shared RAM when Shared == 1 && Paged == 1
; Bit 3: Block (1=sz in Pages, 0=sz in Bytes)   
; Bit 4-6: RESERVED
; Bit 7: IsValid (1=Yes, 0=NO)              Is this structure in use?

AI_ALLOCATED = $01
AI_PAGED     = $02
AI_SHARED    = $04
AI_BLOCK     = $08
AI_VALID     = $80

AI_DEFAULT   = AI_VALID
ALLOC_TASK   = AI_VALID | AI_ALLOCATED
ALLOC_PAGED  = ALLOC_TASK | AI_PAGED
ALLOC_SHARED = ALLOC_PAGED | AI_SHARED

MSG_IN_BUFFER_BASE  = $8000
MSG_OUT_BUFFER_BASE = $9000

MSG_BUFFER_BANK = $F0

.struct     BlockAllocInfo
            status      .byte
            pages       .word                               ; 256-byte pages in this block
            next        .tag Address
            addr        .tag PageAddress
.endstruct

PI_VALID = $01

.struct     BlockInfo
            status      .byte
.endstruct

MMU_INIT:
            jsr         TASK_RAM_INIT
            jmp         SHARED_RAM_INIT

TASK_RAM_INIT:
            lda         T_REGISTER
            cmp         #SYSTEM_TASK_NUM
            beq         :+
            jmp         ERROR_NOT_SYSTEM_TASK

:
            phx
            ldx         #MAX_TASK_NUMBER

@task_loop:
            stx         T_REGISTER

; Initialize the Task RAM for the current task
            ; start at $0400-$7FFF

; Initiazize the Paged RAM space for the current task

; Initialize Message buffer pointers
            ldy         #MAX_TASK_NUMBER

:
            ;lda         ZP_MSG_INP,Y
            ;sta         ZP_MSG_OUTP,Y
            dey
            bpl         :-
            dex
            bpl         @task_loop
            bra         :+
            lda         #SYSTEM_TASK_NUM
            sta         T_REGISTER

:
            plx
            rts

; .X = To Task#, .A = Byte to write
MSG_SEND_BYTE:
            sei
            phy                                             ; Save Y, RAM_BANK and U
            ldy         RAM_BANK_REG
            phy
            ldy         U_REGISTER
            phy
            pha
            lda         T_REGISTER
            SL_N        4
            stx         ZP_M_TEMP
            ora         ZP_M_TEMP
            tax
            ;lda
            pla
            sta         U_REGISTER
            pla
            sta         RAM_BANK_REG
            ply
            cli
            rts

.macro _M_ERROR_RETURN  errno
            lda         #errno
            sec
            rts
.endmacro

ERROR_NOT_SYSTEM_TASK:
            _M_ERROR_RETURN     ERR_NOT_SYSTEM_TASK

ERROR_OUT_OF_MEMORY:
            _M_ERROR_RETURN     ERR_OUT_OF_MEMORY

; IN: .A.Y = Bytes to allocate
; OUT (success): .A.Y = Address of allocation, C = 0
; OUT (failure): .A = ERROR, C = 1
TASK_ALLOC:
            cpy         #0                                  ; Is this a byte alloc or block alloc
            beq         TASK_BYTE_ALLOC
            cmp         #0                                  ; Is this a whole-block allocation?
            beq         TASK_BLOCK_ALLOC
            iny                                             ; Allocate one more page to cover the extra bytes
            tya                                             ; ...and then fall through to do a block allocation

; IN: .A = Pages to allocate for current task
; OUT (success): .A.Y = Address of the allocation structure, C = 0
; OUT (failure): .A = ERROR, C = 1
TASK_BLOCK_ALLOC:
            rts

; IN: .A = Bytes to allocate for current task
; OUT (success): .A.Y = Address of the allocation structure, C = 0
; OUT (failure): .A = ERROR, C = 1
TASK_BYTE_ALLOC:
            rts

TASK_FREE:
            rts

; IN: .A.Y = Bytes to allocate, T must be 0
; OUT (success): .A.Y = Address of shared allocation structure, C = 0
; OUT (out of memory): .A = ERR_OUT_OF_MEMORY, C = 1
;           If T <> 0, .A = ERR_NOT_SYSTEM_TASK, C = 1
SHARED_ALLOC:
            phx
            ldx         T_REGISTER
            cpx         #SYSTEM_TASK_NUM
            beq         :+
            plx
            jmp         ERROR_NOT_SYSTEM_TASK

:
                                                            ; Do the allocation

            plx
            rts

; IN: .A.Y = Address of PageAddress struct that points to location in shared memory to free
SHARED_FREE:
            rts

SHARED_RAM_INIT:
            rts

; IN: .X = Byte to write, .A.Y = Address of PageAddress struct that points to read location in shared memory
SHARED_WRITE:
            sei
            phx                                             ; Push the byte to write
            jsr         SHARED_RW_PAGE_SETUP                ; Clobbers .X.  After, .X = old RAM_BANK, .Y = old U
            pla                                             ; Get it back
            sta         (ZP_M_SP1)                          ; Do the write
            stx         RAM_BANK_REG                        ; Restore prior RAM_BANK
            sty         U_REGISTER                          ; Restore prior Shared RAM Macro-page
            cli
            rts

; IN: .A.Y = Address of PageAddress struct that points to shared memory location to write to
; OUT: .A = Byte read at address
SHARED_READ:
            sei
            phx                                             ; Don't clobber .X
            jsr         SHARED_RW_PAGE_SETUP                ; Clobbers .X.  After, .X = old RAM_BANK, .Y = old U
            lda         (ZP_M_SP1)                          ; Do the read
            stx         RAM_BANK_REG                        ; Restore prior RAM_BANK
            sty         U_REGISTER                          ; Restore prior Shared RAM Macro-page
            plx
            cli
            rts

SHARED_RW_PAGE_SETUP:
            sta         ZP_M_SP1
            sty         ZP_M_SP1 + 1
            ldy         PageAddress::addr + Address::l
            lda         (ZP_M_SP1),Y                        ; Get the LOB of the address
            tax
            iny                                             ; Now the HOB
            lda         (ZP_M_SP1),Y
            tay
            lda         (ZP_M_SP1)                          ; Load the page
            stx         ZP_M_SP1
            sty         ZP_M_SP1 + 1
            tay                                             ; Save the page for use later
            ora         #$F0                                ; Shared Bank transform
            ldx         RAM_BANK_REG                        ; Save bank for restore
            sta         RAM_BANK_REG                        ; Update the RAM_BANK to new Shared bank
            tya                                             ; Get the original page back
            lsr                                             ; Shift right 4-bits
            lsr
            lsr
            lsr
            ldy         U_REGISTER                          ; Save macro-page for restore
            sta         U_REGISTER                          ; Update macro-page[0..3] from PageAddress::page[4..7]
            rts

MEM_TEST:
            PUSH_AXY
            PRINT_CRLF
            stz         RAM_BANK_REG
            stz         T_REGISTER

@task_num_loop:
            lda         #$02
            ldx         #$80                                ; exclude the task serial buffers @ $0200 && $0300
            jsr         TEST_PAGE_RANGE
            ldx         #$A0                                ; end of banked RAM

@ram_bank_loop:
            PRINT_BYTE  RAM_BANK_REG
            PRINT_CHAR  #ASCII_PERIOD
            lda         #$80
            jsr         TEST_PAGE_RANGE
            inc         RAM_BANK_REG
            lda         #NUM_RAM_BANKS
            cmp         RAM_BANK_REG
            bne         @ram_bank_loop
            ;inc         T_REGISTER
            ;lda         #$10
            ;cmp         T_REGISTER
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
            inc         RAM_BANK_REG                        ; increment shared bank
            bne         @shared_bank_loop
            inc         U_REGISTER
            lda         U_REGISTER
            cmp         #$10
            bcc         @shared_banks_loop
            PULL_YXA
            rts

; .A = HOB of first page to test, .X = HOB of last page + 1
TEST_PAGE_RANGE:
            sta         ZP_TEMP_VEC + 1
            PRINT_BYTE
            PRINT_BYTE  #0
            PRINT_CHAR  #ASCII_PERIOD
            txa
            dec
            PRINT_BYTE
            PRINT_BYTE  #$FF
            PRINT_CHAR  #ASCII_COLON
            stz         ZP_TEMP_VEC
            stz         ZP_TEMP

@loop_init:
            ldy         #0

@loop:
            lda         (ZP_TEMP_VEC),Y                     ; save what is in memory (non-descructive)
            sta         ZP_M_SV
            lda         #$AA
@test_it:
            eor         #$FF
            sta         (ZP_TEMP_VEC),Y
            cmp         (ZP_TEMP_VEC),Y
            beq         @next
            lda         #ASCII_BANG
            bra         @write                              ; always, no need to restore value since it isn't storing properly anyway

@next:
            cmp         #$AA
            bne         @test_it
            lda         ZP_M_SV                             ; restore saved value
            sta         (ZP_TEMP_VEC),Y
            iny
            bne         @loop
            lda         #ASCII_PERIOD

@write:
            jsr         WRITE_CHAR 
            stz         ZP_TEMP
            inc         ZP_TEMP_VEC + 1
            cpx         ZP_TEMP_VEC + 1
            bne         @loop_init
            PRINT_CRLF_JMP

; test memory pages, start page in .A, end page in .X
DEEP_PAGE_TEST_RANGE_AX:
            stx         ZP_M_SP2_H
            sta         ZP_M_SP1_H
            bra         DEEP_PAGE_TEST_RANGE

; test a single page in .A
DEEP_PAGE_TEST_A:
            sta         ZP_M_SP1_H

; test a single page in ZP_M_SP1_H
DEEP_PAGE_TEST:
            lda         ZP_M_SP1_H
            sta         ZP_M_SP2_H

; test a range of pages, start in ZP_M_SP1_H, end in ZP_M_SP2_H
DEEP_PAGE_TEST_RANGE:
            PUSH_XY
            stz         ZP_M_SP1_L
@next_page:
            lda         ZP_M_SP1_H
            PRINT_BYTE
            PRINT_BYTE  #0
            PRINT_CHAR  #ASCII_COLON
            PRINT_CRLF
            ldy         #0
            ldx         #0
@next_cell:
            stz         ZP_M_TEMP
            lda         (ZP_M_SP1),Y
            sta         ZP_M_SV                             ; save the old value
            lda         #1
            sta         ZP_M_TEMP2
@loop_start:
            lda         ZP_M_TEMP2
            sta         (ZP_M_SP1),Y                     ; store it
            cmp         (ZP_M_SP1),Y                     ; test it
            bne         :+
            eor         #$FF                                ; test the inverse
            sta         (ZP_M_SP1),Y                     ; store it
            cmp         (ZP_M_SP1),Y                     ; test it
            beq         @shift
:
            lda         ZP_M_TEMP2
            ora         ZP_M_TEMP                           ; add bit to set of bad bits found
            sta         ZP_M_TEMP
@shift:
            asl         ZP_M_TEMP2                          ; Walk the bit forward
            bne         @loop_start
            lda         ZP_M_TEMP                           ; get the set of bad bits we found
            PRINT_BYTE
            inx
            cpx         #$10
            beq         :+
            PRINT_SPACE
            bra         :++
:
            PRINT_CRLF
            ldx         #0
:
            lda         ZP_M_SV
            sta         (ZP_M_SP1),Y                     ; restore the old value
            iny
            bne         @next_cell
            lda         ZP_M_SP1_H
            cmp         ZP_M_SP2_H
            beq         @done
            inc         ZP_M_SP1_H
            jmp         @next_page
@done:
            PULL_YX
            rts
