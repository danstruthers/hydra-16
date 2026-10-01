.debuginfo

; ****************************************************************************
; /dev/ram: the RAM itself, as the CPU selects it, for task 0 only (BIOS ROM page 9; included inside `.scope
; PAGE9`, see all.s).  Served in its client's task, which is always task 0: any other task's open is refused
; (ERR_IO_PERM), so no program reads another's memory through it (docs/plans/PROC.md).  Read-only.
;   $0000000-$007FFFF   task RAM: task t's $0000-$7FFF at t * $8000
;   $0080000-$027FFFF   shared RAM: shared bank ID s at $80000 + s * $2000
;   $0280000-$207FFFF   the RAM modules: module m (0-14), task t's bank b (its bank ID m << 4 | b) at
;                       $280000 + m * $200000 + t * $20000 + b * $2000
; A read gives up to RAW_MAX bytes, and stops at a 256-byte page's end (a short read: read again).  It switches T
; (and the bank) to the RAM's task with IRQs off, and copies straight into task 0's transfer area, which the
; request block's spare bytes (RAW_*) also hold the copy's numbers in: the one RAM every task sees with $00 at the
; IO transfer bank.  The task's own $00 and the zero page bytes the copy uses are put back, and read as they were.

.segment "SYS_P9"

RAW_MAX         = 64                                    ; Bytes a read (with IRQs off: about 45 cycles each)
RAW_BLK         = PAGED_RAM_BASE                        ; Task 0's request block, when mapped (IO_SRV_MAP)
RAW_DATA        = RAW_BLK + IO_BLK_DATA                 ;   and its data area
RAW_TASK        = RAW_BLK + $10                         ; The copy's task (whose T it reads with)
RAW_BANK        = RAW_BLK + $11                         ;   its bank ($00), or $FF: task RAM (when RAW_U is $FF)
RAW_U           = RAW_BLK + $12                         ;   U for a shared bank, or $FF: a module's bank
RAW_SRC         = RAW_BLK + $13                         ;   where (2 bytes)
RAW_N           = RAW_BLK + $15                         ;   how many bytes
RAW_SAVE        = RAW_BLK + $16                         ;   the task's $00, ZP_TEMP_VEC (2): put back after
RAW_T           = RAW_BLK + $19                         ;   scratch
.assert     IO_BLK_CALL < $10 .and RAW_T < RAW_BLK + IO_BLK_NS, error, "RAW_*: the request block's spare bytes"

; IN: .A = request, .X = client, .Y = fid
RAM_SERVE:
            cmp         #H9_CREATE
            bcs         RAM_BAD                             ; (The filesystem's requests)
            cmp         #H9_OPEN
            beq         RAM_OPEN
            cmp         #H9_READ
            beq         RAM_READ
            cmp         #H9_WRITE
            bne         :+
            lda         #ERR_IO_MODE                        ; (Read-only)
            sec
            rts
:
            cmp         #H9_STAT
            bne         :+
            jmp         STAT_ZERO
:
            cmp         #H9_CTL
            beq         RAM_BAD                             ; H9_CLUNK, H9_DUP: nothing to do

RAM_OK:
            lda         #0
            clc
            rts

RAM_BAD:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

; Task 0 only, and the rest of the name (in the data area) empty: /dev/ram.  OUT: .A = the fid (0)
RAM_OPEN:
            txa
            beq         :+
            lda         #ERR_IO_PERM
            sec
            rts
:
            jsr         IO_SRV_MAP
            lda         RAW_DATA
            jsr         IO_SRV_UNMAP
            cmp         #0
            beq         RAM_OK
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

; A read at the fd's offset: the offset as a task and an address, then RAW_COPY.  Past the RAM: none (end of file)
RAM_READ:
            ldx         #SYSTEM_TASK_NUM                    ; (The client: task 0)
            jsr         IO_SRV_MAP
            lda         RAW_BLK + IO_BLK_COUNT              ; How many: the count, RAW_MAX at most, and to the
            beq         :+                                  ;   page's end (0: 256)
            cmp         #RAW_MAX + 1
            bcc         :++
:
            lda         #RAW_MAX
:
            sta         RAW_N
            lda         RAW_BLK + IO_BLK_OFS
            sta         RAW_SRC
            beq         :+
            eor         #$FF                                ; (256 - the offset's low byte)
            inc
            cmp         RAW_N
            bcs         :+
            sta         RAW_N
:
            lda         #$FF
            sta         RAW_U                               ; (Not a shared bank, yet)
            lda         RAW_BLK + IO_BLK_OFS + 3
            bne         @module                             ; (16 MB on: a module's)
            lda         RAW_BLK + IO_BLK_OFS + 2
            cmp         #>(RAW_SHARED >> 8)
            bcs         @shared
            lda         RAW_BLK + IO_BLK_OFS + 1            ; Task RAM: task (bits 15-18), address (0-14)
            asl
            lda         RAW_BLK + IO_BLK_OFS + 2
            rol
            and         #MAX_TASK_NUMBER
            sta         RAW_TASK
            lda         RAW_BLK + IO_BLK_OFS + 1
            and         #$7F
            sta         RAW_SRC + 1
            lda         #$FF
            sta         RAW_BANK
            bra         @copy

@shared:
            cmp         #>(RAW_MODULES >> 8)
            bcs         @module
            sec                                             ; Shared RAM: its bank ID
            sbc         #>(RAW_SHARED >> 8)
            jsr         RAM_BANK_OF                         ; .A = (offset bits 13-20): the shared bank ID
            pha
            lsr
            lsr
            lsr
            lsr
            sta         RAW_U
            pla
            and         #$0F
            ora         #$F0
            sta         RAW_BANK
            stz         RAW_TASK                            ; (Seen from any task: task 0's own T)
            bra         @copy

@module:
            lda         RAW_BLK + IO_BLK_OFS + 2            ; A module: the offset from the first's, in 64K
            sec                                             ;   units (9 bits: RAW_T, and C)
            sbc         #>(RAW_MODULES >> 8)
            sta         RAW_T
            lda         RAW_BLK + IO_BLK_OFS + 3
            sbc         #0
            cmp         #2
            bcs         @past
            lsr                                             ; (C = bit 8: modules 8-14)
            lda         RAW_T
            ror                                             ; .A = the module * 8 + the task / 2 ...
            pha
            and         #$F0
            cmp         #NUM_RAM_MODULES << 4
            pla
            bcs         @past                               ; (Module 15 and on: none)
            lsr                                             ; ... the module (bits 21-24) ...
            lsr
            lsr
            lsr
            asl                                             ;   (<< 4: its bank IDs' upper nibble)
            asl
            asl
            asl
            pha
            lda         RAW_T                               ; ... the task (bits 17-20) ...
            lsr
            and         #MAX_TASK_NUMBER
            sta         RAW_TASK
            lda         RAW_T                               ; ... and its bank (bits 13-16)
            jsr         RAM_BANK_OF
            and         #$0F
            sta         RAW_T
            pla
            ora         RAW_T
            sta         RAW_BANK

@copy:
            jsr         RAW_COPY
            lda         RAW_N
            bra         @count

@past:
            lda         #0                                  ; (End of file)

@count:
            jsr         IO_SRV_COUNT
            jsr         IO_SRV_UNMAP
            jmp         RAM_OK

; An 8K bank's number from the offset (.A = its bits 16-23, its bits 8-15 in the request block), and the address
; in the window ($8000-$9FFF) in RAW_SRC + 1.  OUT: .A = bits 13-20 of the offset.  Modifies: .X
RAM_BANK_OF:
            asl
            asl
            asl
            tax
            lda         RAW_BLK + IO_BLK_OFS + 1
            and         #$1F
            ora         #>PAGED_RAM_BASE
            sta         RAW_SRC + 1
            lda         RAW_BLK + IO_BLK_OFS + 1
            lsr
            lsr
            lsr
            lsr
            lsr
            stx         RAW_T
            ora         RAW_T
            rts

RAW_SHARED      = $80000
RAW_MODULES     = $280000

; Copy RAW_N bytes from RAW_SRC, with T = RAW_TASK and (but for task RAM) its $00 = RAW_BANK and U = RAW_U (a shared
; bank), to the data area (task 0's transfer area, mapped).  IRQs off throughout, and no stack while T is another
; task's: its zero page and stack are what's at $0000-$01FF then.  The task's $00 and the zero page bytes used
; (ZP_TEMP_VEC) are saved in RAW_SAVE and put back, and a copy of them reads as they were.  Modifies: .A, .X, .Y
RAW_COPY:
            php
            sei
            ldx         RAW_TASK
            stx         T_REGISTER                          ; (Its $00, its zero page, from here)
            ldy         RAM_BANK_REG                        ; Its $00 (task RAM mirrors it) ...
            lda         #IO_XFER_BANK
            sta         RAM_BANK_REG                        ; ... and the transfer area seen with it (U = 0)
            sty         RAW_SAVE
            lda         ZP_TEMP_VEC
            sta         RAW_SAVE + 1
            lda         ZP_TEMP_VEC + 1
            sta         RAW_SAVE + 2
            lda         RAW_SRC
            sta         ZP_TEMP_VEC
            lda         RAW_SRC + 1
            sta         ZP_TEMP_VEC + 1
            ldy         #0
            lda         RAW_U                               ; (A shared bank first: its $00 can be $FF too)
            cmp         #$FF
            bne         @shared
            lda         RAW_BANK
            cmp         #$FF
            beq         @task_ram

@module:                                                    ; A module's bank: $00 its, then the transfer
            lda         RAW_BANK                            ;   area's, a byte at a time
            sta         RAM_BANK_REG
            lda         (ZP_TEMP_VEC),Y
            ldx         #IO_XFER_BANK
            stx         RAM_BANK_REG
            sta         RAW_DATA,Y
            iny
            cpy         RAW_N
            bne         @module
            bra         @done

@shared:                                                    ; A shared bank: U and $00 (both read before U
            ldx         RAW_BANK                            ;   changes: the transfer area is U 0's)
            lda         RAW_U
            sta         U_REGISTER
            stx         RAM_BANK_REG
            lda         (ZP_TEMP_VEC),Y
            stz         U_REGISTER
            ldx         #IO_XFER_BANK
            stx         RAM_BANK_REG
            sta         RAW_DATA,Y
            iny
            cpy         RAW_N
            bne         @shared
            bra         @done

@task_ram:
            lda         (ZP_TEMP_VEC),Y
            sta         RAW_DATA,Y
            iny
            cpy         RAW_N
            bne         @task_ram
            lda         RAW_SRC + 1                         ; Its zero page: $00 and the copy's pointer as they
            bne         @done                               ;   were
            ldx         #2

@fix:
            lda         RAW_ZP_USED,X
            sec
            sbc         RAW_SRC
            bcc         @next
            cmp         RAW_N
            bcs         @next
            tay
            lda         RAW_SAVE,X
            sta         RAW_DATA,Y

@next:
            dex
            bpl         @fix

@done:
            lda         RAW_SAVE + 1
            sta         ZP_TEMP_VEC
            lda         RAW_SAVE + 2
            sta         ZP_TEMP_VEC + 1
            lda         RAW_SAVE
            sta         RAM_BANK_REG                        ; Its $00 as it was
            stz         T_REGISTER                          ; Back to task 0, the client
            plp
            rts

RAW_ZP_USED:    .byte   RAM_BANK_REG, ZP_TEMP_VEC, ZP_TEMP_VEC + 1  ; (In RAW_SAVE's order)
