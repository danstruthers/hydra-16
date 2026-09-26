.debuginfo

.segment "MMU"

.struct     Address
            l           .byte
            h           .byte
.endstruct

.struct     PageAddress
            addr        .tag Address
            page        .byte
.endstruct

; status byte =>
; Bit 0: Paged      (1=Paged, 0=Task)               Points to Paged RAM
; Bit 1: Shared     (1=Shared, 0=Task)              Points to Shared RAM when Shared == 1 && Paged == 1
; Bit 2: Block      (1=sz in Pages, 0=sz in Bytes)  Points to 256-byte blocks instead of bytes
; Bit 3: Small      (1=3 or fewer bytes, 0=4+ bytes (or 1+ pages))  Bytes are stored in the allocation structure itself, not in a separate allocation, only valid if bits 0-2 are 0
; Bit 4: Allocated  (1=Allocated, 0=Free)           Is the memory free or allocated
; Bit 5: Readonly   (1=not writable, 0=writable)    Allocation is read-only
; Bit 6: RESERVED
; Bit 7: IsValid    (1=Yes, 0=NO)                   Is this structure in use?

AI_PAGED     = $01
AI_SHARED    = $02
AI_BLOCK     = $04
AI_SMALL     = $08      ; No actual allocation: bytes are stored in addr, offset in bits 0..1 of status
AI_ALLOCATED = $10
AI_READONLY  = $20
AI_VALID     = $80

AI_DEFAULT   = AI_VALID
ALLOC_TASK   = AI_VALID    | AI_ALLOCATED
ALLOC_PAGED  = ALLOC_TASK  | AI_PAGED
ALLOC_SHARED = ALLOC_PAGED | AI_SHARED
ALLOC_SMALL  = ALLOC_TASK  | AI_SMALL

.struct     SmallAllocInfo
            addr        .tag PageAddress
            status      .byte
.endstruct

.struct     AllocInfo
            .tag        SmallAllocInfo
            pages       .word                               ; 256-byte pages in this block
            offset      .word                               ; 
.endstruct

.struct     PageAllocInfo
            block_size  .byte                               ; minimum 2
            next_free   .byte                               ; zero means no further blocks are free
.endstruct

PI_VALID = $01

.struct     BlockInfo
            status      .byte
.endstruct

; ****************************************************************************
; Per-task MMU area (see MMU_PLAN.md)
;
;   Lives at the top of Task RAM, directly below the paged RAM window.  Every
;   task has its own copy, so a task switch (T) swaps the tables in for free.
;   The page just below it is the task system page (IRQ tables, etc.).
;   Task pages are allocated top-down from MMU_PAGE_TOP to MMU_PAGE_BOTTOM.

MMU_TASK_PAGES   = 2                                        ; Size of the MMU area in 256-byte pages
MMU_AREA         = $8000 - (MMU_TASK_PAGES * $100)          ; $7E00 for 2 pages
MMU_SYS_PAGE     = (>MMU_AREA) - 1                          ; Task system page ($7D for 2 pages)
MMU_PAGE_TOP     = MMU_SYS_PAGE - 1                         ; Highest allocatable task page
MMU_PAGE_BOTTOM  = $08                                      ; $0000-$07FF: ZP, stack and BUFFERS
MMU_BANK_TOP     = $EF                                      ; Task RAM banks are $00-$EF
MMU_VERSION      = $01                                      ; Non-zero status == initialized
PAGED_RAM_BASE   = $8000                                    ; RAM bank window ($8000-$9FFF, bank in RAM_BANK_REG)

.assert     MMU_SYS_PAGE >= $78, error, "MMU_TASK_PAGES too large for top page-map byte pre-mark"

.struct     MmuHeader
            status      .byte                               ; MMU_VERSION when initialized
            page_map    .res 16                             ; 1 bit per page $00-$7F (1 = in use)
            bank_map    .res 30                             ; 1 bit per bank $00-$EF (1 = in use)
            chunk_heads .res 6                              ; First page of each chunk size class (0 = none)
            page_ends   .res 16                             ; 1 bit per page, marks the last page of each run
            bank_ends   .res 30                             ; 1 bit per bank, marks the last bank of each run
            low_water   .byte                               ; Lowest allocated page (MMU_SYS_PAGE when empty)
            handles     .byte                               ; Start of the handle table (rest of the MMU area)
.endstruct

MMU_HDR          = MMU_AREA                                 ; Absolute access: MMU_HDR + MmuHeader::field
MMU_PAGE_MAP     = MMU_HDR + MmuHeader::page_map
MMU_PAGE_ENDS    = MMU_HDR + MmuHeader::page_ends
MMU_BANK_MAP     = MMU_HDR + MmuHeader::bank_map
MMU_BANK_ENDS    = MMU_HDR + MmuHeader::bank_ends

MMU_BIT_MASKS:
            .byte       $01, $02, $04, $08, $10, $20, $40, $80

; Initialize the MMU for all tasks, and the shared RAM tables.  Must be called from the system task.
; OUT: C = 0 on success; C = 1 and .A = ERR_NOT_SYSTEM_TASK if not called from the system task
MMU_INIT:
            jsr         TASK_RAM_INIT
            bcs         :+
            jmp         SHARED_RAM_INIT
:
            rts

; Initialize the MMU area of every task.  Must be called from the system task.
TASK_RAM_INIT:
            lda         T_REGISTER
            cmp         #SYSTEM_TASK_NUM
            beq         :+
            jmp         ERROR_NOT_SYSTEM_TASK

:
            php                                             ; Save caller's I flag
            sei                                             ; No IRQs while switching tasks
            phx
            jsr         MMU_PROBE_MODULES                   ; Installed RAM modules -> ZP_M_MODS (task 0)
            ldx         #MAX_TASK_NUMBER

@task_loop:
            stz         T_REGISTER                          ; Copy task 0's ZP_M_MODS to task X
            lda         ZP_M_MODS
            ldy         ZP_M_MODS + 1
            stx         T_REGISTER                          ; Quick switch to task X
            sta         ZP_M_MODS
            sty         ZP_M_MODS + 1
            jsr         MM_TASK_INIT                        ; Preserves .X
            dex
            bpl         @task_loop
            lda         #SYSTEM_TASK_NUM
            sta         T_REGISTER                          ; Back to the system task
            plx
            plp                                             ; Restore caller's I flag
            clc
            rts

; Find the installed RAM modules (16 banks each: module m = banks m*16 - m*16+15), by write/read-back
; of the first byte of each module's first bank (the byte is restored).  Run in task 0 with IRQs off.
; OUT: ZP_M_MODS = bit m set if module m is installed
; Modifies: .A, .X, .Y
MMU_PROBE_MODULES:
            stz         ZP_M_MODS
            stz         ZP_M_MODS + 1
            lda         RAM_BANK_REG
            pha
            ldx         #0                                  ; Module#

@loop:
            txa
            asl                                             ; First bank of the module
            asl
            asl
            asl
            sta         RAM_BANK_REG
            ldy         PAGED_RAM_BASE                      ; Save the byte
            lda         #$55
            sta         PAGED_RAM_BASE
            lda         PAGED_RAM_BASE
            cmp         #$55
            bne         @next                               ; Not installed
            lda         #$AA
            sta         PAGED_RAM_BASE
            lda         PAGED_RAM_BASE
            cmp         #$AA
            bne         @next                               ; Not installed
            sty         PAGED_RAM_BASE                      ; Restore the byte
            phx
            txa
            lsr
            lsr
            lsr
            tay                                             ; .Y = ZP_M_MODS byte
            txa
            and         #7
            tax
            lda         MMU_BIT_MASKS,X
            ora         ZP_M_MODS,Y
            sta         ZP_M_MODS,Y
            plx

@next:
            inx
            cpx         #NUM_RAM_MODULES
            bne         @loop
            pla
            sta         RAM_BANK_REG
            rts

; Reset the current task's MMU area: all task pages and banks become free, except banks of RAM
; modules that aren't installed (ZP_M_MODS, see MMU_PROBE_MODULES).
; Also used on task completion (MM_TASK_RESET).
; Modifies: .A, .Y
MM_TASK_INIT:
            phx
            lda         #0
            tay

@clear:
.repeat     MMU_TASK_PAGES, I
            sta         MMU_AREA + (I * $100),Y
.endrepeat
            iny
            bne         @clear
            lda         #$FF                                ; Pages $00-$07: ZP, stack and BUFFERS
            sta         MMU_HDR + MmuHeader::page_map
            lda         #($FF << (MMU_SYS_PAGE & 7)) & $FF  ; System page and MMU area pages
            sta         MMU_HDR + MmuHeader::page_map + 15
            lda         #MMU_SYS_PAGE
            sta         MMU_HDR + MmuHeader::low_water
            lda         ZP_M_MODS                           ; Mark the banks of missing modules in use
            sta         ZP_M_TEMP
            lda         ZP_M_MODS + 1
            sta         ZP_M_TEMP2
            ldx         #0                                  ; Bank map byte: module * 2

@modules:
            lsr         ZP_M_TEMP2                          ; C = next module's installed bit
            ror         ZP_M_TEMP
            bcs         @installed
            lda         #$FF
            sta         MMU_BANK_MAP,X
            sta         MMU_BANK_MAP + 1,X

@installed:
            inx
            inx
            cpx         #NUM_RAM_MODULES * 2
            bne         @modules
            lda         #MMU_VERSION
            sta         MMU_HDR + MmuHeader::status
            plx
            rts

; ****************************************************************************
; Task page and bank allocators

; Allocate a run of contiguous 256-byte task pages, top-down.
; IN: .A = number of pages (1+)
; OUT (success): .A = first (lowest) page of the run, C = 0
; OUT (failure): .A = ERROR, C = 1
; Modifies: .A
MM_PAGE_ALLOC:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            jsr         MM_PAGE_MAPS_SETUP
            ldx         #MMU_PAGE_TOP
            ldy         #MMU_PAGE_BOTTOM
            jsr         BM_ALLOC_RUN
            bcs         @done
            cmp         MMU_HDR + MmuHeader::low_water      ; New lowest page?
            bcs         @ok
            sta         MMU_HDR + MmuHeader::low_water

@ok:
            clc

@done:
            PULL_YX
            jmp         MM_RETURN

; Free a run of task pages allocated with MM_PAGE_ALLOC.
; IN: .A = first page of the run
; OUT (success): C = 0
; OUT (failure): .A = ERROR, C = 1
; Modifies: .A
MM_PAGE_FREE:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            cmp         #MMU_PAGE_BOTTOM
            bcc         @bad_arg
            cmp         #MMU_PAGE_TOP + 1
            bcs         @bad_arg
            pha
            jsr         MM_PAGE_MAPS_SETUP
            pla
            ldx         #MMU_PAGE_TOP
            jsr         BM_FREE_RUN
            bcs         @done
            cmp         MMU_HDR + MmuHeader::low_water      ; Freed the lowest run?
            bne         @done_ok
            jsr         MM_UPDATE_LOW_WATER

@done_ok:
            clc

@done:
            PULL_YX
            jmp         MM_RETURN

@bad_arg:
            lda         #ERR_MEM_BAD_ARG
            sec
            bra         @done

; Allocate a run of contiguous 8K task RAM banks, top-down.
; IN: .A = number of banks (1+)
; OUT (success): .A = first (lowest) bank of the run, C = 0
; OUT (failure): .A = ERROR, C = 1
; Modifies: .A
MM_BANK_ALLOC:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            jsr         MM_BANK_MAPS_SETUP
            ldx         #MMU_BANK_TOP
            ldy         #0
            jsr         BM_ALLOC_RUN
            PULL_YX
            jmp         MM_RETURN

; Free a run of task RAM banks allocated with MM_BANK_ALLOC.
; IN: .A = first bank of the run
; OUT (success): C = 0
; OUT (failure): .A = ERROR, C = 1
; Modifies: .A
MM_BANK_FREE:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            cmp         #MMU_BANK_TOP + 1
            bcs         @bad_arg
            pha
            jsr         MM_BANK_MAPS_SETUP
            pla
            ldx         #MMU_BANK_TOP
            jsr         BM_FREE_RUN

@done:
            PULL_YX
            jmp         MM_RETURN

@bad_arg:
            lda         #ERR_MEM_BAD_ARG
            sec
            bra         @done

; Common exit for routines that start with php/sei: restore the caller's I flag, keep C
MM_RETURN:
            bcs         :+
            plp
            clc
            rts
:
            plp
            sec
            rts

; Recompute the low-water mark: the lowest allocated page between MMU_PAGE_BOTTOM and MMU_PAGE_TOP,
; or MMU_SYS_PAGE if none.  Expects ZP_M_BM to point at the page map.
; Modifies: .A, .X, .Y
MM_UPDATE_LOW_WATER:
            ldx         MMU_HDR + MmuHeader::low_water

@loop:
            cpx         #MMU_SYS_PAGE
            bcs         @store
            txa
            jsr         BM_BIT
            and         (ZP_M_BM),Y
            bne         @store                              ; Found an allocated page
            inx
            bra         @loop

@store:
            stx         MMU_HDR + MmuHeader::low_water
            rts

; Point ZP_M_BM / ZP_M_BE at the current task's page map / page run-end map
MM_PAGE_MAPS_SETUP:
            LOAD_ADDR   MMU_PAGE_MAP, ZP_M_BM
            LOAD_ADDR   MMU_PAGE_ENDS, ZP_M_BE
            rts

; Point ZP_M_BM / ZP_M_BE at the current task's bank map / bank run-end map
MM_BANK_MAPS_SETUP:
            LOAD_ADDR   MMU_BANK_MAP, ZP_M_BM
            LOAD_ADDR   MMU_BANK_ENDS, ZP_M_BE
            rts

; ****************************************************************************
; Generic bitmap run allocator.  Used for task pages and banks, and later for shared banks.
;   ZP_M_BM -> allocation bitmap (1 = in use), ZP_M_BE -> run-end bitmap (1 = last bit of a run)
;   Bit n lives in byte (n >> 3), mask MMU_BIT_MASKS[n & 7]

; IN: .A = bit number
; OUT: .Y = byte offset into the bitmap, .A = bit mask
; Modifies: .A, .Y
BM_BIT:
            pha
            lsr
            lsr
            lsr
            tay
            pla
            phx
            and         #7
            tax
            lda         MMU_BIT_MASKS,X
            plx
            rts

; Find, mark and return the highest run of .A free bits in [.Y, .X]
; IN: .A = run length (1+), .X = highest bit to consider, .Y = lowest bit to consider
; OUT (success): .A = first (lowest) bit of the run, C = 0
; OUT (failure): .A = ERROR, C = 1
; Modifies: .A, .X, .Y
BM_ALLOC_RUN:
            cmp         #0
            beq         @bad_arg
            sta         ZP_M_CNT
            sty         ZP_M_LO
            stz         ZP_M_RUN

@loop:
            txa
            jsr         BM_BIT
            and         (ZP_M_BM),Y
            bne         @used
            inc         ZP_M_RUN                            ; One more free bit in this run
            lda         ZP_M_RUN
            cmp         ZP_M_CNT
            beq         @found
            bra         @next

@used:
            stz         ZP_M_RUN                            ; Run broken, start over below this bit

@next:
            cpx         ZP_M_LO
            beq         @no_mem
            dex
            bra         @loop

@found:
            stx         ZP_M_LO                             ; .X = first bit of the run, save it for return

@mark:
            txa
            jsr         BM_BIT
            ora         (ZP_M_BM),Y
            sta         (ZP_M_BM),Y
            inx
            dec         ZP_M_CNT
            bne         @mark
            dex                                             ; Last bit of the run
            txa
            jsr         BM_BIT
            ora         (ZP_M_BE),Y
            sta         (ZP_M_BE),Y
            lda         ZP_M_LO
            clc
            rts

@no_mem:
            lda         #ERR_OUT_OF_MEMORY
            sec
            rts

@bad_arg:
            lda         #ERR_MEM_BAD_ARG
            sec
            rts

; Free the run of bits starting at .A, up to and including the next run-end bit
; IN: .A = first bit of the run, .X = highest valid bit
; OUT (success): .A = first bit of the run, C = 0
; OUT (failure): .A = ERR_MEM_NOT_ALLOC, C = 1 (nothing is changed if the first bit is free)
; Modifies: .A, .X, .Y
BM_FREE_RUN:
            stx         ZP_M_LO                             ; Highest valid bit
            sta         ZP_M_RUN                            ; Save first bit for return
            tax
            jsr         BM_BIT
            and         (ZP_M_BM),Y
            beq         @not_alloc                          ; First bit isn't allocated

@loop:
            txa
            jsr         BM_BIT
            eor         #$FF
            and         (ZP_M_BM),Y                         ; Clear the allocated bit
            sta         (ZP_M_BM),Y
            txa
            jsr         BM_BIT
            and         (ZP_M_BE),Y
            bne         @end_found
            cpx         ZP_M_LO
            beq         @end_found                          ; Never run past the end of the map
            inx
            bra         @loop

@end_found:
            txa
            jsr         BM_BIT
            eor         #$FF
            and         (ZP_M_BE),Y                         ; Clear the run-end bit
            sta         (ZP_M_BE),Y
            lda         ZP_M_RUN
            clc
            rts

@not_alloc:
            lda         #ERR_MEM_NOT_ALLOC
            sec
            rts

;
;   .A.Y = MemAlloc addr
;   .X   = Byte offset
;   C    = 1 = Use offset, 0 = Read byte at .A.Y
MEM_READ:
            ; .A.Y 
            ; If IS_SMALL, check to see if C && .X < 3
            php                                             ; Save caller's I flag (and C)
            sei
            sta         ZP_M_SP1_L
            sty         ZP_M_SP1_H
            ldy         #SmallAllocInfo::status
            lda         (ZP_M_SP1), y
            sta         ZP_M_TEMP
            bbr7        ZP_M_TEMP, @err_not_valid
            bbr4        ZP_M_TEMP, @err_not_alloc
            bbs3        ZP_M_TEMP, @is_small
            lda         #ERR_MEM_NOT_SUPPORTED              ; TODO: non-small reads
            bra         @ret_err

@is_small:
            bcs         @x_offset_small
            lda         #3
            and         ZP_M_TEMP
            SKIPNEXT

@x_offset_small:
            txa

@do_read_small:
            tay
            lda         (ZP_M_SP1), y

@ret_ok:
            plp                                             ; Restore caller's I flag
            clc
            rts

@err_not_alloc:
            lda         #ERR_MEM_NOT_ALLOC
            bra         @ret_err

@err_not_valid:
            lda         #ERR_MEM_NOT_VALID

@ret_err:
            plp                                             ; Restore caller's I flag
            sec
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

; IN: .X = Byte to write, .A.Y = Address of PageAddress struct that points to shared memory location to write to
SHARED_WRITE:
            php                                             ; Save caller's I flag
            sei
            phx                                             ; Push the byte to write
            jsr         SHARED_RW_PAGE_SETUP                ; Clobbers .X.  After, .X = old RAM_BANK, .Y = old U
            pla                                             ; Get it back
            sta         (ZP_M_SP1)                          ; Do the write
            stx         RAM_BANK_REG                        ; Restore prior RAM_BANK
            sty         U_REGISTER                          ; Restore prior Shared RAM Macro-page
            plp                                             ; Restore caller's I flag
            rts

; IN: .A.Y = Address of PageAddress struct that points to read location in shared memory
; OUT: .A = Byte read at address
SHARED_READ:
            php                                             ; Save caller's I flag
            sei
            phx                                             ; Don't clobber .X
            jsr         SHARED_RW_PAGE_SETUP                ; Clobbers .X.  After, .X = old RAM_BANK, .Y = old U
            lda         (ZP_M_SP1)                          ; Do the read
            stx         RAM_BANK_REG                        ; Restore prior RAM_BANK
            sty         U_REGISTER                          ; Restore prior Shared RAM Macro-page
            plx
            plp                                             ; Restore caller's I flag
            rts

SHARED_RW_PAGE_SETUP:
            sta         ZP_M_SP1
            sty         ZP_M_SP1 + 1
            ldy         #PageAddress::page
            lda         (ZP_M_SP1),Y                        ; Load the page
            pha                                             ; ...and save it for use later
            ldy         #PageAddress::addr + Address::h
            lda         (ZP_M_SP1),Y                        ; Get the HOB of the address
            tax
            ldy         #PageAddress::addr + Address::l
            lda         (ZP_M_SP1),Y                        ; Now the LOB
            sta         ZP_M_SP1
            stx         ZP_M_SP1 + 1
            pla                                             ; Get the page back
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

; Memory Copy
; ZP_TEMP_VEC: From, ZP_TEMP_VEC2: To, .A.Y: Size
MEM_COPY:
            phx
            tax
            beq         :+                                  ; Whole pages?  .Y is already the pass count
            iny                                             ; Partial page counts as one more pass of .X
:
            tya
            beq         @done                               ; copy zero bytes?  Done!
            PRINT_CRLF
            PRINT_BYTE  ZP_TEMP_VEC + 1
            PRINT_BYTE  ZP_TEMP_VEC
            PRINT_CHAR  #ASCII_DASH, #ASCII_GT
            PRINT_BYTE  ZP_TEMP_VEC2 + 1
            PRINT_BYTE  ZP_TEMP_VEC2

@loop:
            lda         (ZP_TEMP_VEC)
            sta         (ZP_TEMP_VEC2)
            PRINT_CHAR  #ASCII_PERIOD
            inc         ZP_TEMP_VEC
            bne         :+
            inc         ZP_TEMP_VEC + 1
:
            inc         ZP_TEMP_VEC2
            bne         :+
            inc         ZP_TEMP_VEC2 + 1
:
            dex
            bne         @loop
            dey
            bne         @loop

@done:
            plx
            rts

; TESTS
MEM_TEST:
            PUSH_AXY
            PRINT_CRLF
            ;stz         T_REGISTER

@task_num_loop:
            stz         RAM_BANK_REG
            lda         #$04
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
            PRINT_CHAR
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

; .A.Y: address of alloc struct
; .X: byte number
; C: 0 = read byte at .A.Y, 1 = index block with X
MEM_READ_BYTE:
; Test AI_BLOCK and return error if set
; Test AI_SMALL
; Test AI_PAGED
; Test AI_SHARED

;AI_ALLOCATED = $01
;AI_PAGED     = $02
;AI_SHARED    = $04
;AI_BLOCK     = $08
;AI_READONLY  = $10
;AI_SMALL     = $20      ; No actual allocation: bytes are stored in addr
;AI_VALID     = $80
