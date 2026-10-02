.debuginfo

; ****************************************************************************
; /dev/ram: the RAM itself, as the CPU selects it, for task 0 only (BIOS ROM page 9; included inside `.scope
; PAGE9`, see all.s).  Served in its client's task, which is always task 0: any other task's open is refused
; (ERR_IO_PERM), so no program reads another's memory through it (docs/plans/PROC.md).  Read-only.
;   $0000000-$007FFFF   task RAM: task t's $0000-$7FFF at t * $8000
;   $0080000-$027FFFF   shared RAM: shared bank ID s at $80000 + s * $2000
;   $0280000-$207FFFF   the RAM modules: module m (0-14), task t's bank b (its bank ID m << 4 | b) at
;                       $280000 + m * $200000 + t * $20000 + b * $2000
; A read gives up to MC_MAX bytes, and stops at a 256-byte page's end (a short read: read again).  It's a copy
; with T the RAM's task (MEM_COPY, below, which /proc/N/mem and ram use too).

.segment "SYS_P9"

RAW_BLK         = PAGED_RAM_BASE                        ; Task 0's request block, when mapped (IO_SRV_MAP)
RAW_DATA        = RAW_BLK + IO_BLK_DATA                 ;   and its data area

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

; A read at the fd's offset: the offset as a task and an address, then MEM_READ.  Past the RAM: none (end of file)
RAM_READ:
            ldx         #SYSTEM_TASK_NUM                    ; (The client: task 0)
            jsr         IO_SRV_MAP
            jsr         MEM_COUNT                           ; MC_N: the count, MC_MAX at most, and to the page's end
            lda         RAW_BLK + IO_BLK_OFS
            sta         MC_ADDR
            lda         #$FF
            sta         MC_U                                ; (Not a shared bank, yet)
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
            sta         MC_TASK
            lda         RAW_BLK + IO_BLK_OFS + 1
            and         #$7F
            sta         MC_ADDR + 1
            lda         #$FF
            sta         MC_BANK
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
            sta         MC_U
            pla
            and         #$0F
            ora         #$F0
            sta         MC_BANK
            stz         MC_TASK                             ; (Seen from any task: task 0's own T)
            bra         @copy

@module:
            lda         RAW_BLK + IO_BLK_OFS + 2            ; A module: the offset from the first's, in 64K
            sec                                             ;   units (9 bits: MC_T, and C)
            sbc         #>(RAW_MODULES >> 8)
            sta         MC_T
            lda         RAW_BLK + IO_BLK_OFS + 3
            sbc         #0
            cmp         #2
            bcs         @past
            lsr                                             ; (C = bit 8: modules 8-14)
            lda         MC_T
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
            lda         MC_T                                ; ... the task (bits 17-20) ...
            lsr
            and         #MAX_TASK_NUMBER
            sta         MC_TASK
            lda         MC_T                                ; ... and its bank (bits 13-16)
            jsr         RAM_BANK_OF
            and         #$0F
            sta         MC_T
            pla
            ora         MC_T
            sta         MC_BANK

@copy:
            jsr         MEM_READ
            lda         MC_N
            bra         @count

@past:
            lda         #0                                  ; (End of file)

@count:
            jsr         IO_SRV_COUNT
            jmp         RAM_OK

; An 8K bank's number from the offset (.A = its bits 16-23, its bits 8-15 in the request block), and the address
; in the window ($8000-$9FFF) in MC_ADDR + 1.  OUT: .A = bits 13-20 of the offset.  Modifies: .X
RAM_BANK_OF:
            asl
            asl
            asl
            tax
            lda         RAW_BLK + IO_BLK_OFS + 1
            and         #$1F
            ora         #>PAGED_RAM_BASE
            sta         MC_ADDR + 1
            lda         RAW_BLK + IO_BLK_OFS + 1
            lsr
            lsr
            lsr
            lsr
            lsr
            stx         MC_T
            ora         MC_T
            rts

RAW_SHARED      = $80000
RAW_MODULES     = $280000

; ****************************************************************************
; Another task's memory, copied to or from the client's data area (for /dev/ram, /proc/N/mem and /proc/N/ram).
; The client's transfer area is mapped (IO_SRV_MAP: ZP_IO_REQ, its $00 at its transfer bank, U = 0), and its
; MC_TASK, MC_BANK, MC_U, MC_ADDR and MC_N set.  T is the task's for the copy, with IRQs off throughout and no
; stack use (its zero page and stack are what's at $0000-$01FF then), and its $00 at the client's transfer bank,
; so the data area and MC_AREA are seen; its own $00 and the zero page bytes the copy uses (MC_ZP) are saved in
; MC_SAVE and put back, and read (or are written) as if they hadn't been used.
;   MC_BANK = $FF:  its address space as it is (task RAM, its paged ROM bank): MC_ADDR straight
;   MC_U = $FF:     its bank MC_BANK at $8000, switched in for each byte (the transfer bank uses the window too)
;   else:           shared bank MC_BANK of U = MC_U, the same way
; Modifies: .A, .X, .Y
MC_ZP           = ZP_TEMP_VEC                           ; The task's zero page bytes the copy uses:
MC_ZP_N         = 8                                     ;   ZP_TEMP_VEC-ZP_TEMP_VEC4
MC_P_MEM        = MC_ZP                                 ; The memory's pointer
MC_P_DATA       = MC_ZP + 2                             ;   and the data area's
MC_Z_BANK       = MC_ZP + 4                             ; MC_BANK
MC_Z_XBANK      = MC_ZP + 5                             ; MC_XBANK
MC_Z_U          = MC_ZP + 6                             ; MC_U
.assert     ZP_TEMP_VEC4 + 2 = MC_ZP + MC_ZP_N .and RAM_BANK_REG = 0, error, "MC_ZP: ZP_TEMP_VEC-ZP_TEMP_VEC4, and $00"

; Copy MC_N bytes from the task's memory to the data area
MEM_READ:
            lda         #0
            bra         MEM_COPY

; Copy MC_N bytes from the data area to the task's memory
MEM_WRITE:
            lda         #1

MEM_COPY:
            sta         MC_WRITE
            lda         T_REGISTER                          ; The client: this task, its transfer bank and
            and         #$0F                                ;   data area (mapped)
            sta         MC_CLIENT
            lda         RAM_BANK_REG
            sta         MC_XBANK
            ldx         ZP_IO_REQ + 1
            inx
            stx         MC_DATA + 1
            stz         MC_DATA
            php
            sei
            ldx         MC_TASK
            stx         T_REGISTER                          ; (Its $00, its zero page, from here)
            ldy         RAM_BANK_REG                        ; Its $00 (task RAM mirrors it) ...
            sta         RAM_BANK_REG                        ; ... and the client's transfer bank seen with it
            sty         MC_SAVE
            ldx         #MC_ZP_N - 1
:
            lda         MC_ZP,X
            sta         MC_SAVE + 1,X
            dex
            bpl         :-
            lda         MC_ADDR
            sta         MC_P_MEM
            lda         MC_ADDR + 1
            sta         MC_P_MEM + 1
            lda         MC_DATA
            sta         MC_P_DATA
            lda         MC_DATA + 1
            sta         MC_P_DATA + 1
            lda         MC_BANK
            sta         MC_Z_BANK
            lda         MC_XBANK
            sta         MC_Z_XBANK
            lda         MC_U
            sta         MC_Z_U
            ldy         #0
            cmp         #$FF                                ; (A shared bank first: its MC_BANK can be $FF too)
            beq         :+
            jmp         @shared
:
            lda         MC_BANK
            cmp         #$FF
            bne         @bank
            lda         MC_WRITE                            ; Its address space as it is: straight (the transfer
            bne         @w_direct                           ;   bank stays at $8000)

@r_direct:
            lda         (MC_P_MEM),Y
            sta         (MC_P_DATA),Y
            iny
            cpy         MC_N
            bne         @r_direct
            lda         MC_P_MEM + 1                        ; Its zero page: $00 and the copy's bytes as they were
            beq         :+
            jmp         @done
:
            ldx         #MC_ZP_N                            ; (MC_SAVE + .X: 0 is $00, 1 on MC_ZP + .X - 1)

@fix:
            txa
            beq         :+
            clc
            adc         #MC_ZP - 1
:
            sec
            sbc         MC_P_MEM
            bcc         @fix_next
            cmp         MC_N
            bcs         @fix_next
            tay
            lda         MC_SAVE,X
            sta         (MC_P_DATA),Y

@fix_next:
            dex
            bpl         @fix
            jmp         @done

@w_direct:
            lda         MC_P_MEM + 1
            beq         @w_zero

@w_straight:
            lda         (MC_P_DATA),Y
            sta         (MC_P_MEM),Y
            iny
            cpy         MC_N
            bne         @w_straight
            jmp         @done

@w_zero:                                                    ; Its zero page: $00 and the copy's bytes go to
            tya                                             ;   MC_SAVE (put in place as the copy ends)
            clc
            adc         MC_P_MEM                            ; (The address)
            ldx         #0
            cmp         #RAM_BANK_REG
            beq         @w_saved
            sec
            sbc         #MC_ZP
            cmp         #MC_ZP_N
            bcs         @w_plain
            tax
            inx

@w_saved:
            lda         (MC_P_DATA),Y
            sta         MC_SAVE,X
            bra         @w_next

@w_plain:
            lda         (MC_P_DATA),Y
            sta         (MC_P_MEM),Y

@w_next:
            iny
            cpy         MC_N
            bne         @w_zero
            bra         @done

@bank:                                                      ; A bank: its $00, then the transfer bank's, for
            lda         MC_WRITE                            ;   each byte
            bne         @w_bank

@r_bank:
            ldx         MC_Z_BANK
            stx         RAM_BANK_REG
            lda         (MC_P_MEM),Y
            ldx         MC_Z_XBANK
            stx         RAM_BANK_REG
            sta         (MC_P_DATA),Y
            iny
            cpy         MC_N
            bne         @r_bank
            bra         @done

@w_bank:
            lda         (MC_P_DATA),Y
            ldx         MC_Z_BANK
            stx         RAM_BANK_REG
            sta         (MC_P_MEM),Y
            ldx         MC_Z_XBANK
            stx         RAM_BANK_REG
            iny
            cpy         MC_N
            bne         @w_bank
            bra         @done

@shared:                                                    ; A shared bank: U and $00 (and U back to 0 before
            lda         MC_WRITE                            ;   the transfer bank: it's U 0's)
            bne         @w_shared

@r_shared:
            ldx         MC_Z_U
            stx         U_REGISTER
            ldx         MC_Z_BANK
            stx         RAM_BANK_REG
            lda         (MC_P_MEM),Y
            stz         U_REGISTER
            ldx         MC_Z_XBANK
            stx         RAM_BANK_REG
            sta         (MC_P_DATA),Y
            iny
            cpy         MC_N
            bne         @r_shared
            bra         @done

@w_shared:
            lda         (MC_P_DATA),Y
            ldx         MC_Z_U
            stx         U_REGISTER
            ldx         MC_Z_BANK
            stx         RAM_BANK_REG
            sta         (MC_P_MEM),Y
            stz         U_REGISTER
            ldx         MC_Z_XBANK
            stx         RAM_BANK_REG
            iny
            cpy         MC_N
            bne         @w_shared

@done:
            ldx         #MC_ZP_N - 1                        ; Its zero page as it was (or as written)
:
            lda         MC_SAVE + 1,X
            sta         MC_ZP,X
            dex
            bpl         :-
            lda         MC_SAVE
            ldx         MC_CLIENT
            sta         RAM_BANK_REG                        ; Its $00 as it was
            stx         T_REGISTER                          ; Back to the client
            plp
            rts

; MC_N = how many bytes a copy: the request's count (ZP_IO_REQ: mapped), MC_MAX at most, and not past the end of
; the offset's 256-byte page.  OUT: .A = MC_N.  Modifies: .Y
MEM_COUNT:
            ldy         #IO_BLK_COUNT + 1
            lda         (ZP_IO_REQ),Y
            bne         @most                               ; (256 or more)
            dey
            lda         (ZP_IO_REQ),Y
            beq         @most                               ; (A count is 1-256: 0 here is 256)
            cmp         #MC_MAX + 1
            bcc         :+

@most:
            lda         #MC_MAX
:
            sta         MC_N
            ldy         #IO_BLK_OFS
            lda         (ZP_IO_REQ),Y
            beq         @done
            eor         #$FF                                ; (256 - the offset's low byte)
            inc
            cmp         MC_N
            bcs         @done
            sta         MC_N

@done:
            lda         MC_N
            rts
