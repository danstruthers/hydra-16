.debuginfo

.segment "MMU"

; Handle table entry status byte =>
; Bit 0: Paged      (1=Paged, 0=Task)               Points to Paged RAM (8K banks at $8000)
; Bit 1: Shared     (1=Shared, 0=Task)              Points to Shared RAM when Shared == 1 && Paged == 1
; Bit 2: Block      (1=Pages, 0=other)              Points to a run of 256-byte task RAM pages
; Bit 3: Small      (1=3 or fewer bytes)            Bytes are stored in the handle entry itself, not in a separate
;                                                   allocation; bits 0-1 then hold the length (1-3)
; Bit 4: Allocated  (1=Allocated, 0=Free)           Is the memory free or allocated
; Bit 5: Readonly   (1=not writable, 0=writable)    Allocation is read-only
; Bit 6: Locked     (1=Locked)                      MM_LOCK handed out a raw pointer; can't be freed
; Bit 7: IsValid    (1=Yes, 0=NO)                   Is this structure in use?

AI_PAGED     = $01
AI_SHARED    = $02
AI_BLOCK     = $04
AI_SMALL     = $08      ; No actual allocation: bytes are stored in the entry, length in bits 0..1 of status
AI_ALLOCATED = $10
AI_READONLY  = $20
AI_LOCKED    = $40
AI_VALID     = $80

AI_SMALL_LEN = $03      ; Length mask for AI_SMALL entries
AI_IN_USE    = AI_VALID | AI_ALLOCATED

; Handle table entry (4 bytes).  Small allocations keep their data in addr_l, addr_h, bank.
.struct     Handle
            addr_l      .byte                               ; Start address of the allocation
            addr_h      .byte
            bank        .byte                               ; First RAM bank (AI_PAGED)
            status      .byte                               ; AI_* bits; 0 = free entry
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
            page_floor  .byte                               ; Lowest page MM_PAGE_ALLOC may hand out (MM_SET_FLOOR)
            handles     .byte                               ; Start of the handle table (rest of the MMU area)
.endstruct

MMU_HDR          = MMU_AREA                                 ; Absolute access: MMU_HDR + MmuHeader::field
MMU_PAGE_MAP     = MMU_HDR + MmuHeader::page_map
MMU_PAGE_ENDS    = MMU_HDR + MmuHeader::page_ends
MMU_BANK_MAP     = MMU_HDR + MmuHeader::bank_map
MMU_BANK_ENDS    = MMU_HDR + MmuHeader::bank_ends
MMU_LOW_WATER    = MMU_HDR + MmuHeader::low_water
MMU_HANDLE_TBL   = MMU_HDR + MmuHeader::handles             ; Entry for handle h: MMU_HANDLE_TBL + (h - 1) * 4
MMU_MAX_HANDLES  = ($8000 - MMU_HANDLE_TBL) / .sizeof(Handle)   ; 103 for 2 pages
MMU_CHUNK_HEADS  = MMU_HDR + MmuHeader::chunk_heads         ; First chunk page of each size class (0 = none)

; Chunk pages: a 256-byte task RAM page split into chunks of one size (4, 8, 16, 32 or 64 bytes).  The
; first chunk holds the page header; free chunks are linked through their first byte (the in-page offset
; of the next free chunk, 0 = none).
MMU_CHUNK_MIN    = 4                                        ; Smallest chunk (class 0)
MMU_CHUNK_MAX    = 64                                       ; Largest chunk; bigger allocations take whole pages
MMU_CHUNK_CLASSES = 5                                       ; 4, 8, 16, 32, 64

.struct     ChunkPage
            size        .byte                               ; Chunk size
            free_count  .byte                               ; Free chunks in this page
            free_head   .byte                               ; Offset of the first free chunk (0 = none)
            next_page   .byte                               ; Next chunk page of the same size (0 = none)
.endstruct

.assert     .sizeof(ChunkPage) <= MMU_CHUNK_MIN, error, "Chunk page header must fit in the first chunk"
.assert     MMU_CHUNK_CLASSES <= 6, error, "MmuHeader::chunk_heads has 6 entries"

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
            lda         #MMU_PAGE_BOTTOM
            sta         MMU_HDR + MmuHeader::page_floor
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
            ldy         MMU_HDR + MmuHeader::page_floor     ; Never below the page floor
            cpy         #MMU_PAGE_TOP + 1
            bcs         @no_mem                             ; (BM_ALLOC_RUN needs lowest <= highest)
            ldx         #MMU_PAGE_TOP
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

@no_mem:
            lda         #ERR_OUT_OF_MEMORY
            sec
            bra         @done

; Set the page floor: MM_PAGE_ALLOC (and so chunk and page allocations) will only hand out pages at or
; above it.  Lets a task keep the space above its own data free to grow into (HyForth's dictionary).
; The floor can't go above the lowest page already allocated.
; IN: .A = page (raised to MMU_PAGE_BOTTOM if below it)
; OUT (success): C = 0
; OUT (failure): .A = ERR_OUT_OF_MEMORY, C = 1 (pages below .A are already allocated; floor unchanged)
; Preserves .X, .Y
MM_SET_FLOOR:
            php                                             ; Save caller's I flag
            sei
            cmp         #MMU_PAGE_BOTTOM
            bcs         :+
            lda         #MMU_PAGE_BOTTOM
:
            cmp         MMU_HDR + MmuHeader::low_water
            beq         @set
            bcs         @no_mem

@set:
            sta         MMU_HDR + MmuHeader::page_floor
            clc
            jmp         MM_RETURN

@no_mem:
            lda         #ERR_OUT_OF_MEMORY
            sec
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

; Point ZP_M_BM / ZP_M_BE at the current task's page map / page run-end map.  Preserves .A
MM_PAGE_MAPS_SETUP:
            pha
            LOAD_ADDR   MMU_PAGE_MAP, ZP_M_BM
            LOAD_ADDR   MMU_PAGE_ENDS, ZP_M_BE
            pla
            rts

; Point ZP_M_BM / ZP_M_BE at the current task's bank map / bank run-end map.  Preserves .A
MM_BANK_MAPS_SETUP:
            pha
            LOAD_ADDR   MMU_BANK_MAP, ZP_M_BM
            LOAD_ADDR   MMU_BANK_ENDS, ZP_M_BE
            pla
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

.macro _M_ERROR_RETURN  errno
            lda         #errno
            sec
            rts
.endmacro

ERROR_NOT_SYSTEM_TASK:
            _M_ERROR_RETURN     ERR_NOT_SYSTEM_TASK

ERROR_OUT_OF_MEMORY:
            _M_ERROR_RETURN     ERR_OUT_OF_MEMORY

; ****************************************************************************
; Handles (per task).  A handle is a 1-byte index (1 - MMU_MAX_HANDLES) into the current task's handle
; table; 0 is never a valid handle.  All calls: C = 0 on success, C = 1 with the error in .A.
;
;   Allocation tiers:
;       1-3 bytes       AI_SMALL: the bytes are kept in the handle entry itself
;       4-64 bytes      Chunk (no tier bit): a 4, 8, 16, 32 or 64-byte chunk of a shared chunk page
;       65+ bytes       AI_BLOCK: a run of 256-byte task RAM pages ($0800-$7CFF, top-down)
;       .X = AI_PAGED   AI_PAGED: a run of 8K RAM banks, seen at $8000-$9FFF (MM_LOCK selects the bank)

; Allocate memory for the current task.
; IN: .A.Y = size in bytes (1 - $FFFF; .A = low byte), .X = 0 or AI_PAGED (8K RAM banks)
; OUT (success): .A = handle, C = 0
; OUT (failure): .A = ERR_MEM_BAD_ARG, ERR_MEM_NO_HANDLES or ERR_OUT_OF_MEMORY, C = 1
; Preserves .X, .Y
MM_ALLOC:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            sta         ZP_M_SZ1                            ; Size low
            sty         ZP_M_TEMP2                          ; Size high
            stx         ZP_M_SV                             ; Flags
            ora         ZP_M_TEMP2
            bne         :+
            jmp         @bad_arg                            ; Zero bytes
:
            jsr         MM_NEW_HANDLE                       ; Free entry -> ZP_M_HP, ZP_M_HANDLE
            bcc         :+
            jmp         @done
:
            lda         ZP_M_SV
            and         #AI_PAGED
            bne         @banks
            lda         ZP_M_TEMP2
            bne         @pages
            lda         ZP_M_SZ1
            cmp         #AI_SMALL_LEN + 1
            bcs         @not_small

; 1-3 bytes: kept in the entry itself
            ora         #AI_IN_USE | AI_SMALL               ; Status = length + flags
            ldy         #Handle::status
            sta         (ZP_M_HP),Y
            lda         #0
            ldy         #Handle::addr_l                     ; Clear the data bytes
            sta         (ZP_M_HP),Y
            iny
            sta         (ZP_M_HP),Y
            iny
            sta         (ZP_M_HP),Y
            bra         @ok

; 4-64 bytes: a chunk of a chunk page
@not_small:
            cmp         #MMU_CHUNK_MAX + 1
            bcs         @pages
            jsr         MM_CHUNK_ALLOC                      ; ZP_M_CP + 1 = page, ZP_M_COFS = offset
            bcs         @done
            ldy         #Handle::addr_l
            lda         ZP_M_COFS
            sta         (ZP_M_HP),Y
            iny
            lda         ZP_M_CP + 1
            sta         (ZP_M_HP),Y
            iny
            lda         #0
            sta         (ZP_M_HP),Y
            lda         #AI_IN_USE                          ; No tier bit: a chunk
            bra         @set_status

; 65+ bytes: whole task RAM pages
@pages:
            lda         ZP_M_TEMP2                          ; Pages = size high + (size low <> 0)
            ldx         ZP_M_SZ1
            beq         :+
            inc                                             ; ($FFxx wraps to 0: rejected as a bad size)
:
            jsr         MM_PAGE_ALLOC                       ; .A = first page
            bcs         @done
            ldy         #Handle::addr_h
            sta         (ZP_M_HP),Y
            lda         #0
            ldy         #Handle::addr_l
            sta         (ZP_M_HP),Y
            ldy         #Handle::bank
            sta         (ZP_M_HP),Y
            lda         #AI_IN_USE | AI_BLOCK
            bra         @set_status

; AI_PAGED: whole 8K RAM banks
@banks:
            lda         ZP_M_SZ1                            ; Banks = ((size - 1) >> 13) + 1
            cmp         #1
            lda         ZP_M_TEMP2
            sbc         #0                                  ; High byte of size - 1
            lsr
            lsr
            lsr
            lsr
            lsr
            inc
            jsr         MM_BANK_ALLOC                       ; .A = first bank
            bcs         @done
            ldy         #Handle::bank
            sta         (ZP_M_HP),Y
            lda         #<PAGED_RAM_BASE
            ldy         #Handle::addr_l
            sta         (ZP_M_HP),Y
            lda         #>PAGED_RAM_BASE
            iny
            sta         (ZP_M_HP),Y
            lda         #AI_IN_USE | AI_PAGED

@set_status:
            ldy         #Handle::status                     ; Status last: it makes the entry live
            sta         (ZP_M_HP),Y

@ok:
            lda         ZP_M_HANDLE
            clc

@done:
            PULL_YX
            jmp         MM_RETURN

@bad_arg:
            lda         #ERR_MEM_BAD_ARG
            sec
            bra         @done

; Free an allocation and its handle.
; IN: .A = handle
; OUT (success): C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID or ERR_MEM_LOCKED, C = 1
; Preserves .X, .Y
MM_FREE:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            jsr         MM_HANDLE_PTR                       ; .A = status
            bcs         @done
            bit         #AI_LOCKED
            bne         @locked
            bit         #AI_PAGED
            beq         :+
            ldy         #Handle::bank
            lda         (ZP_M_HP),Y
            jsr         MM_BANK_FREE
            bcs         @done
            bra         @free_entry
:
            bit         #AI_SMALL
            bne         @free_entry                         ; AI_SMALL: nothing else to free
            bit         #AI_BLOCK
            beq         @chunk
            ldy         #Handle::addr_h
            lda         (ZP_M_HP),Y
            jsr         MM_PAGE_FREE
            bcs         @done
            bra         @free_entry

@chunk:
            ldy         #Handle::addr_h
            lda         (ZP_M_HP),Y
            sta         ZP_M_CP + 1                         ; Chunk page
            ldy         #Handle::addr_l
            lda         (ZP_M_HP),Y                         ; Chunk offset
            jsr         MM_CHUNK_FREE
            bcs         @done

@free_entry:
            lda         #0
            ldy         #Handle::status
            sta         (ZP_M_HP),Y
            clc

@done:
            PULL_YX
            jmp         MM_RETURN

@locked:
            lda         #ERR_MEM_LOCKED
            sec
            bra         @done

; Read a byte of an allocation.
; IN: .A = handle, .Y = offset (0-255; within the allocation's size for AI_SMALL)
; OUT (success): .A = byte, C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID or ERR_MEM_BAD_ARG, C = 1
; Preserves .X, .Y
MM_READ:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            sty         ZP_M_TEMP                           ; Offset
            jsr         MM_HANDLE_PTR                       ; .A = status
            bcs         @done
            jsr         MM_ACCESS_SETUP                     ; ZP_M_SP1 = data, bank selected
            bcs         @done
            ldy         ZP_M_TEMP
            lda         (ZP_M_SP1),Y
            ldy         ZP_M_SV
            sty         RAM_BANK_REG                        ; Restore the RAM bank
            clc

@done:
            PULL_YX
            jmp         MM_RETURN

; Write a byte of an allocation.
; IN: .A = handle, .Y = offset (0-255; within the allocation's size for AI_SMALL), .X = byte
; OUT (success): C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID or ERR_MEM_BAD_ARG, C = 1
; Preserves .X, .Y
MM_WRITE:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            sty         ZP_M_TEMP                           ; Offset
            jsr         MM_HANDLE_PTR                       ; .A = status
            bcs         @done
            jsr         MM_ACCESS_SETUP                     ; ZP_M_SP1 = data, bank selected
            bcs         @done
            ldy         ZP_M_TEMP
            txa
            sta         (ZP_M_SP1),Y
            ldy         ZP_M_SV
            sty         RAM_BANK_REG                        ; Restore the RAM bank
            clc

@done:
            PULL_YX
            jmp         MM_RETURN

; Get a raw pointer to an allocation, for speed.  For AI_PAGED allocations this also selects the
; allocation's first RAM bank; the old bank is returned for MM_UNLOCK.  A locked allocation can't be
; freed.  The pointer stays valid until MM_UNLOCK (allocations don't move).
; IN: .A = handle
; OUT (success): .A.Y = pointer (.A = low byte), .X = previous RAM bank (pass it to MM_UNLOCK), C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID, C = 1
MM_LOCK:
            php                                             ; Save caller's I flag
            sei
            jsr         MM_HANDLE_PTR                       ; .A = status
            bcs         @done
            ora         #AI_LOCKED
            ldy         #Handle::status
            sta         (ZP_M_HP),Y
            stz         ZP_M_TEMP                           ; Offset 0 (always valid)
            jsr         MM_ACCESS_SETUP                     ; ZP_M_SP1 = data, bank selected
            ldx         ZP_M_SV                             ; Previous RAM bank
            lda         ZP_M_SP1
            ldy         ZP_M_SP1 + 1
            clc

@done:
            jmp         MM_RETURN

; Release a pointer from MM_LOCK and restore the RAM bank.
; IN: .A = handle, .X = RAM bank to restore (from MM_LOCK)
; OUT (success): C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID, C = 1 (the RAM bank is restored anyway)
; Preserves .X, .Y
MM_UNLOCK:
            php                                             ; Save caller's I flag
            sei
            phy
            stx         RAM_BANK_REG
            jsr         MM_HANDLE_PTR                       ; .A = status
            bcs         @done
            and         #<~AI_LOCKED
            ldy         #Handle::status
            sta         (ZP_M_HP),Y
            clc

@done:
            ply
            jmp         MM_RETURN

; Allocate a chunk from the current task's chunk pages, starting a new chunk page if the size class has
; no free chunk.
; IN: .A = bytes (MMU_CHUNK_MIN - MMU_CHUNK_MAX)
; OUT (success): ZP_M_CP + 1 = chunk page, ZP_M_COFS = chunk offset, C = 0
; OUT (failure): .A = ERR_OUT_OF_MEMORY, C = 1
; Modifies: .A, .X, .Y
MM_CHUNK_ALLOC:
            jsr         MM_CHUNK_CLASS                      ; ZP_M_CLS, ZP_M_CSZ
            stz         ZP_M_CP
            ldx         ZP_M_CLS
            lda         MMU_CHUNK_HEADS,X                   ; First chunk page of the class

@find_page:
            beq         @new_page                           ; No page with a free chunk
            sta         ZP_M_CP + 1
            ldy         #ChunkPage::free_count
            lda         (ZP_M_CP),Y
            bne         @take
            ldy         #ChunkPage::next_page
            lda         (ZP_M_CP),Y
            bra         @find_page

@new_page:
            lda         #1
            jsr         MM_PAGE_ALLOC                       ; .A = page
            bcs         @done
            sta         ZP_M_CP + 1
            lda         ZP_M_CSZ
            sta         (ZP_M_CP)                           ; ChunkPage::size
            ldy         #ChunkPage::free_head
            sta         (ZP_M_CP),Y                         ; The first chunk after the header
            ldx         #0                                  ; Free chunk count

@link:                                                      ; .A = this chunk's offset
            tay
            clc
            adc         ZP_M_CSZ                            ; Next chunk (C = 1 past the end of the page)
            bcc         :+
            lda         #0                                  ; Last chunk: end of the list
:
            sta         (ZP_M_CP),Y
            inx
            cmp         #0
            bne         @link
            txa
            ldy         #ChunkPage::free_count
            sta         (ZP_M_CP),Y
            ldx         ZP_M_CLS                            ; Put the page at the head of the class list
            lda         MMU_CHUNK_HEADS,X
            ldy         #ChunkPage::next_page
            sta         (ZP_M_CP),Y
            lda         ZP_M_CP + 1
            sta         MMU_CHUNK_HEADS,X

@take:
            ldy         #ChunkPage::free_head
            lda         (ZP_M_CP),Y
            sta         ZP_M_COFS                           ; The chunk
            tay
            lda         (ZP_M_CP),Y                         ; Its link: the next free chunk
            ldy         #ChunkPage::free_head
            sta         (ZP_M_CP),Y
            ldy         #ChunkPage::free_count
            lda         (ZP_M_CP),Y
            dec
            sta         (ZP_M_CP),Y
            clc

@done:
            rts

; Free a chunk.  A chunk page that becomes completely free goes back to the page allocator.
; IN: ZP_M_CP + 1 = chunk page, .A = chunk offset
; OUT (success): C = 0
; OUT (failure): .A = ERROR, C = 1
; Modifies: .A, .X, .Y
MM_CHUNK_FREE:
            stz         ZP_M_CP
            sta         ZP_M_COFS
            ldy         #ChunkPage::free_head               ; Link the chunk in at the head of the free list
            lda         (ZP_M_CP),Y
            ldy         ZP_M_COFS
            sta         (ZP_M_CP),Y
            lda         ZP_M_COFS
            ldy         #ChunkPage::free_head
            sta         (ZP_M_CP),Y
            ldy         #ChunkPage::free_count
            lda         (ZP_M_CP),Y
            inc
            sta         (ZP_M_CP),Y
            sta         ZP_M_COFS                           ; Free chunks now
            lda         (ZP_M_CP)                           ; ChunkPage::size
            sta         ZP_M_CSZ
            ldx         #0                                  ; Chunks in a page (after the header)

@count:
            inx
            clc
            adc         ZP_M_CSZ
            bcc         @count
            cpx         ZP_M_COFS
            beq         @release                            ; Every chunk is free
            clc
            rts

; Take the page off its class list and give it back
@release:
            lda         ZP_M_CSZ
            jsr         MM_CHUNK_CLASS                      ; ZP_M_CLS
            ldx         ZP_M_CLS
            ldy         #ChunkPage::next_page
            lda         MMU_CHUNK_HEADS,X
            cmp         ZP_M_CP + 1
            bne         @find_prev
            lda         (ZP_M_CP),Y                         ; It's the first page of the list
            sta         MMU_CHUNK_HEADS,X
            bra         @give_back

@find_prev:
            stz         ZP_M_CPREV
            cmp         #0
            beq         @give_back                          ; Empty list (shouldn't happen)

@next_prev:                                                 ; .A = a page on the list
            sta         ZP_M_CPREV + 1
            lda         (ZP_M_CPREV),Y                      ; Its next page
            beq         @give_back                          ; Not on the list (shouldn't happen)
            cmp         ZP_M_CP + 1
            bne         @next_prev
            lda         (ZP_M_CP),Y                         ; prev.next_page = page.next_page
            sta         (ZP_M_CPREV),Y

@give_back:
            lda         ZP_M_CP + 1
            jmp         MM_PAGE_FREE

; Chunk size class for a size.
; IN: .A = bytes (MMU_CHUNK_MIN - MMU_CHUNK_MAX)
; OUT: ZP_M_CLS = class index, ZP_M_CSZ = chunk size
; Modifies: .X, .Y; preserves .A
MM_CHUNK_CLASS:
            ldx         #0
            ldy         #MMU_CHUNK_MIN

@loop:
            sty         ZP_M_CSZ
            cmp         ZP_M_CSZ
            beq         @found
            bcc         @found
            pha
            tya
            asl
            tay
            pla
            inx
            bra         @loop

@found:
            stx         ZP_M_CLS
            rts

; Find the handle of the current task's chunk or page allocation that starts at an address.
; (Not for AI_SMALL or AI_PAGED allocations: they have no unique address.)
; IN: .A.Y = address (.A = low byte)
; OUT (success): .A = handle, C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID, C = 1
; Preserves .X, .Y
MM_FIND:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            sta         ZP_M_SP1                            ; The address
            sty         ZP_M_SP1 + 1
            LOAD_ADDR   MMU_HANDLE_TBL, ZP_M_HP
            lda         #1
            sta         ZP_M_HANDLE

@loop:
            ldy         #Handle::status
            lda         (ZP_M_HP),Y
            and         #AI_IN_USE | AI_SMALL | AI_PAGED
            cmp         #AI_IN_USE                          ; In use, and a chunk or page run
            bne         @next
            ldy         #Handle::addr_l
            lda         (ZP_M_HP),Y
            cmp         ZP_M_SP1
            bne         @next
            iny
            lda         (ZP_M_HP),Y
            cmp         ZP_M_SP1 + 1
            bne         @next
            lda         ZP_M_HANDLE
            clc
            bra         @done

@next:
            lda         ZP_M_HP                             ; Next entry
            clc
            adc         #.sizeof(Handle)
            sta         ZP_M_HP
            bcc         :+
            inc         ZP_M_HP + 1
:
            inc         ZP_M_HANDLE
            lda         ZP_M_HANDLE
            cmp         #MMU_MAX_HANDLES + 1
            bne         @loop
            lda         #ERR_MEM_NOT_VALID
            sec

@done:
            PULL_YX
            jmp         MM_RETURN

; Find a free handle table entry in the current task.
; OUT (success): ZP_M_HANDLE = handle, ZP_M_HP = its entry, C = 0
; OUT (failure): .A = ERR_MEM_NO_HANDLES, C = 1
; Modifies: .A, .Y
MM_NEW_HANDLE:
            LOAD_ADDR   MMU_HANDLE_TBL, ZP_M_HP
            lda         #1
            sta         ZP_M_HANDLE
            ldy         #Handle::status

@loop:
            lda         (ZP_M_HP),Y
            beq         @found
            lda         ZP_M_HP                             ; Next entry
            clc
            adc         #.sizeof(Handle)
            sta         ZP_M_HP
            bcc         :+
            inc         ZP_M_HP + 1
:
            inc         ZP_M_HANDLE
            lda         ZP_M_HANDLE
            cmp         #MMU_MAX_HANDLES + 1
            bne         @loop
            lda         #ERR_MEM_NO_HANDLES
            sec
            rts

@found:
            clc
            rts

; Point ZP_M_HP at a handle's entry, and check that it's in use.
; IN: .A = handle
; OUT (success): .A = entry status, ZP_M_HP = entry, ZP_M_HANDLE = handle, C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID, C = 1
; Modifies: .A, .Y
MM_HANDLE_PTR:
            sta         ZP_M_HANDLE
            cmp         #0
            beq         @bad
            cmp         #MMU_MAX_HANDLES + 1
            bcs         @bad
            dec                                             ; Entry = MMU_HANDLE_TBL + (handle - 1) * 4
            stz         ZP_M_HP + 1
            asl
            rol         ZP_M_HP + 1
            asl
            rol         ZP_M_HP + 1
            clc
            adc         #<MMU_HANDLE_TBL
            sta         ZP_M_HP
            lda         ZP_M_HP + 1
            adc         #>MMU_HANDLE_TBL
            sta         ZP_M_HP + 1
            ldy         #Handle::status
            lda         (ZP_M_HP),Y
            and         #AI_IN_USE
            cmp         #AI_IN_USE
            bne         @bad
            lda         (ZP_M_HP),Y
            clc
            rts

@bad:
            lda         #ERR_MEM_NOT_VALID
            sec
            rts

; Point ZP_M_SP1 at an allocation's data and select its RAM bank (AI_PAGED).  The current RAM bank is
; saved in ZP_M_SV, for the caller to restore.
; IN: .A = entry status, ZP_M_HP = entry, ZP_M_TEMP = offset to check (AI_SMALL: must be < length)
; OUT (success): C = 0
; OUT (failure): .A = ERR_MEM_BAD_ARG, C = 1
; Modifies: .A, .Y; preserves .X
MM_ACCESS_SETUP:
            ldy         RAM_BANK_REG
            sty         ZP_M_SV
            bit         #AI_SMALL
            beq         @not_small
            and         #AI_SMALL_LEN
            cmp         ZP_M_TEMP
            beq         @bad_arg                            ; Offset >= length
            bcc         @bad_arg
            lda         ZP_M_HP                             ; The data is in the entry itself
            sta         ZP_M_SP1
            lda         ZP_M_HP + 1
            sta         ZP_M_SP1 + 1
            clc
            rts

@not_small:
            bit         #AI_PAGED
            beq         :+
            ldy         #Handle::bank
            lda         (ZP_M_HP),Y
            sta         RAM_BANK_REG
:
            sta         ZP_M_CLS                            ; Keep the status (.X must be preserved)
            ldy         #Handle::addr_l
            lda         (ZP_M_HP),Y
            sta         ZP_M_SP1
            iny
            lda         (ZP_M_HP),Y
            sta         ZP_M_SP1 + 1
            lda         ZP_M_CLS
            and         #AI_PAGED | AI_BLOCK
            bne         @ok                                 ; Pages and banks: any 8-bit offset is inside
            lda         ZP_M_SP1 + 1                        ; Chunk: offset must be < the chunk size
            sta         ZP_M_CP + 1
            stz         ZP_M_CP
            lda         (ZP_M_CP)                           ; ChunkPage::size
            cmp         ZP_M_TEMP
            beq         @bad_arg
            bcc         @bad_arg

@ok:
            clc
            rts

@bad_arg:
            lda         #ERR_MEM_BAD_ARG
            sec
            rts

; ****************************************************************************
; Reset a task's memory: everything it allocated is freed, without walking its handles.
;   1. Its MMU area is re-initialized (all its task pages, chunks, banks and handles are freed)
;   2. Its shared memory references are dropped (banks nobody else references are freed)
;   3. The message rings it sends or receives on are emptied
;   4. Its IRQ / S/W interrupt handlers are unregistered
; Called when a task completes (TASK_START).  The task must not be running (or be the calling task).
; IN: .A = task
; Preserves .A, .X, .Y
MM_TASK_RESET:
            php                                             ; Save caller's I flag
            sei
            PUSH_AXY
            and         #$0F
            sta         ZP_TC_TASK
            LOAD_ADDR   MM_TASK_INIT, ZP_TC_VEC
            jsr         TASK_CALL                           ; Re-initialize its MMU area, in the task
            lda         ZP_TC_TASK
            jsr         SH_RESET_TASK
            jsr         MSG_RESET_TASK
            jsr         IRQ_UNREGISTER_TASK
            PULL_YXA
            plp                                             ; Restore caller's I flag
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
