.debuginfo

.segment "SHARED"

; ****************************************************************************
; Shared memory (see MMU_PLAN.md)
;
;   256 shared 8K banks: shared bank ID = U << 4 | (bank & $0F), seen at $8000-$9FFF with RAM_BANK_REG =
;   $F0-$FF.  The system data lives in shared bank ID $00 (U = 0, bank $F0: SYS_BANK, _M_SYS_ENTER):
;       $8200-$821F  SH_MAP     shared bank bitmap (1 = in use)
;       $8220-$823F  SH_ENDS    run-end bitmap (last bank of each allocation)
;       $8400-$87FF  SH_HANDLES shared handle table: 255 entries of ShHandle
;       $8800-$88FF  IO_DEV_TABLE the IO device table (io.s)
;       $8900-$8CFB  SH_REF_TBL references' far pointers (SH_REF, fp.s), by shared handle
;   Bank IDs $00 (system) and $09 (IO transfer areas) are reserved, as are the banks of any U macro-page
;   whose RAM isn't installed, and the banks of any RAM chip that failed the POST (ZP_M_BAD_SH: chip c
;   holds bank IDs 4c - 4c+3 of every U).
;
;   A shared handle is a 1-byte index (1-255) that any task can use, so it can be passed to another task.
;   Each task that uses it holds a reference (SH_ALLOC / SH_ATTACH); the banks are freed when the last
;   reference goes (SH_DETACH, or MM_TASK_RESET of the task).  Only tasks holding a reference can read or
;   write through the handle.  All calls: C = 0 on success, C = 1 with the error in .A.

SH_MAP              = $8200
SH_ENDS             = $8220
SH_HANDLES          = $8400
SH_MAX_HANDLES      = 255
SH_REF_MARK         = $FF                                   ; ShHandle::count of a reference (SH_REF, fp.s): no
                                                            ;   banks (no allocation can have 255)
SH_REF_TBL          = $8900                                 ; References' far pointers: 255 of FarPtr, by handle
SH_FIRST_FREE_ID    = $01                                   ; (ID $00: system data; $09, the IO transfer
                                                            ;   areas, is reserved in the map)
SH_WINDOW           = PAGED_RAM_BASE                        ; Where SH_LOCK maps a shared allocation
SH_PROBE_ADDR       = $9FFF                                 ; Probe byte (bank $F0 of each U)
SH_PROBE_MARK       = $50                                   ; Probe marker: SH_PROBE_MARK + U

; Set up the shared memory tables: find which U macro-pages have RAM, clear the bitmaps and handle
; table, and reserve the system and IO transfer banks.  Called by MMU_INIT at boot.
SHARED_RAM_INIT:
            php                                             ; Save caller's I flag
            sei
            PUSH_AXY
            _M_SYS_ENTER                                    ; Save RAM bank / U; select shared bank ID $00

; Probe: write a marker into every U's bank $F0, highest U first, so if U decoding aliases, the
; lower (real) macro-page's marker wins and the alias reads back the wrong marker.
            ldx         #15

@mark:
            stx         U_REGISTER
            txa
            ora         #SH_PROBE_MARK
            sta         SH_PROBE_ADDR
            dex
            bpl         @mark
            stz         ZP_TEMP_VEC                         ; Present macro-pages, bit = U
            stz         ZP_TEMP_VEC + 1
            ldx         #15

@check:
            stx         U_REGISTER
            txa
            ora         #SH_PROBE_MARK
            cmp         SH_PROBE_ADDR
            bne         @next                               ; Missing (or an alias)
            phx
            txa
            lsr
            lsr
            lsr
            tay                                             ; .Y = mask byte
            txa
            and         #7
            tax
            lda         MMU_BIT_MASKS,X
            ora         ZP_TEMP_VEC,Y
            sta         ZP_TEMP_VEC,Y
            plx

@next:
            dex
            bpl         @check
            ldy         #SYS_SHARED_U
            sty         U_REGISTER                          ; Back to shared bank ID $00

            lda         #0                                  ; Clear the bitmaps and handle table
            tay

@clear:
            sta         SH_MAP,Y
            sta         SH_HANDLES,Y
            sta         SH_HANDLES + $100,Y
            sta         SH_HANDLES + $200,Y
            sta         SH_HANDLES + $300,Y
            sta         IO_DEV_TABLE,Y                      ; The IO device table ($8800, io.s)
            iny
            bne         @clear
            lda         ZP_M_BAD_SH                         ; Chips that failed the POST (post_ram.s): chip c
            and         #$03                                ;   = bank IDs 4c - 4c+3 of every U, so a nibble
            tax                                             ;   of each U's two SH_MAP bytes
            lda         SH_CHIP_MASKS,X
            sta         ZP_TEMP_VEC2                        ; Bank IDs $x0-$x7: chips 0, 1
            lda         ZP_M_BAD_SH
            lsr
            lsr
            and         #$03
            tax
            lda         SH_CHIP_MASKS,X
            sta         ZP_TEMP_VEC2 + 1                    ; Bank IDs $x8-$xF: chips 2, 3
            ldx         #0                                  ; Reserve the banks of missing macro-pages and bad chips

@macro_pages:
            lda         #$FF
            tay
            lsr         ZP_TEMP_VEC + 1                     ; C = this U's present bit
            ror         ZP_TEMP_VEC
            bcc         @reserve                            ; Missing: all of its banks
            lda         ZP_TEMP_VEC2                        ; Present: the banks on bad chips
            ldy         ZP_TEMP_VEC2 + 1

@reserve:
            sta         SH_MAP,X
            tya
            sta         SH_MAP + 1,X
            inx
            inx
            cpx         #32
            bne         @macro_pages
            lda         #$01                                ; Reserve bank IDs $00 (system data) and
            ora         SH_MAP                              ;   $09 (IO transfer areas)
            sta         SH_MAP
            lda         #$02
            ora         SH_MAP + 1
            sta         SH_MAP + 1
            _M_SYS_LEAVE
            PULL_YXA
            plp                                             ; Restore caller's I flag
            rts

SH_CHIP_MASKS:  .byte   $00, $0F, $F0, $FF                  ; 2 chips' bad bits -> SH_MAP byte (4 bank IDs each)

; Allocate shared memory (whole 8K banks); the calling task holds the first reference.
; IN: .A.Y = size in bytes (1 - $FFFF; .A = low byte)
; OUT (success): .A = shared handle, C = 0
; OUT (failure): .A = ERR_MEM_BAD_ARG, ERR_MEM_NO_HANDLES or ERR_OUT_OF_MEMORY, C = 1
; Preserves .X, .Y
SH_ALLOC:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            sta         ZP_M_SZ1                            ; Size low
            sty         ZP_M_TEMP2                          ; Size high
            ora         ZP_M_TEMP2
            beq         @bad_arg                            ; Zero bytes
            _M_SYS_ENTER                                    ; Select shared bank ID $00
            jsr         SH_NEW_HANDLE                       ; Free entry -> ZP_M_HP, ZP_M_HANDLE
            bcs         @leave
            lda         ZP_M_SZ1                            ; Banks = ((size - 1) >> 13) + 1
            cmp         #1
            lda         ZP_M_TEMP2
            sbc         #0
            lsr
            lsr
            lsr
            lsr
            lsr
            inc
            sta         ZP_M_SV
            jsr         SH_MAPS_SETUP
            ldx         #$FF
            ldy         #SH_FIRST_FREE_ID
            jsr         BM_ALLOC_RUN                        ; .A = first bank ID
            bcs         @leave
            ldy         #ShHandle::bank
            sta         (ZP_M_HP),Y
            lda         #0
            ldy         #ShHandle::mask_lo
            sta         (ZP_M_HP),Y
            iny
            sta         (ZP_M_HP),Y
            lda         T_REGISTER
            sta         ZP_M_TEMP
            jsr         SH_SET_TASK_BIT                     ; The caller's reference
            lda         ZP_M_SV
            ldy         #ShHandle::count                    ; Count last: it makes the entry live
            sta         (ZP_M_HP),Y
            lda         ZP_M_HANDLE
            clc

@leave:
            _M_SYS_LEAVE

@done:
            PULL_YX
            jmp         MM_RETURN

@bad_arg:
            lda         #ERR_MEM_BAD_ARG
            sec
            bra         @done

; Take a reference to a shared allocation for the calling task (e.g. a handle received in a message).
; IN: .A = shared handle
; OUT (success): C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID, C = 1
; Preserves .X, .Y
SH_ATTACH:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            _M_SYS_ENTER                                    ; Select shared bank ID $00
            jsr         SH_HANDLE_PTR
            bcs         @leave
            lda         T_REGISTER
            sta         ZP_M_TEMP
            jsr         SH_SET_TASK_BIT
            clc

@leave:
            _M_SYS_LEAVE
            PULL_YX
            jmp         MM_RETURN

; Drop the calling task's reference; the banks are freed when no task holds a reference.
; IN: .A = shared handle
; OUT (success): C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID, C = 1 (no such handle, or the task has no reference)
; Preserves .X, .Y
SH_DETACH:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            _M_SYS_ENTER                                    ; Select shared bank ID $00
            jsr         SH_HANDLE_PTR
            bcs         @leave
            lda         T_REGISTER
            sta         ZP_M_TEMP
            jsr         SH_DROP_TASK_REF

@leave:
            _M_SYS_LEAVE
            PULL_YX
            jmp         MM_RETURN

; Read a byte of a shared allocation (the calling task must hold a reference).
; IN: .A = shared handle, .Y = offset (0-255, in the first bank)
; OUT (success): .A = byte, C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID, C = 1
; Preserves .X, .Y
SH_READ:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            sty         ZP_M_COFS
            _M_SYS_ENTER                                    ; Select shared bank ID $00
            jsr         SH_ACCESS_SETUP                     ; Selects the allocation's first bank
            bcs         @leave
            bvs         @ref
            ldy         ZP_M_COFS
            lda         SH_WINDOW,Y
            clc

@leave:
            _M_SYS_LEAVE                                    ; Restores the RAM bank and U
            PULL_YX
            jmp         MM_RETURN

@ref:                                                       ; A reference: through its far pointer
            ldy         ZP_M_COFS
            jsr         FP_READ
            bra         @leave

; Write a byte of a shared allocation (the calling task must hold a reference).
; IN: .A = shared handle, .Y = offset (0-255, in the first bank), .X = byte
; OUT (success): C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID, C = 1
; Preserves .X, .Y
SH_WRITE:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            sty         ZP_M_COFS
            stx         ZP_M_SZ1                            ; The byte (SH_ACCESS_SETUP uses .X)
            _M_SYS_ENTER                                    ; Select shared bank ID $00
            jsr         SH_ACCESS_SETUP                     ; Selects the allocation's first bank
            bcs         @leave
            bvs         @ref
            ldy         ZP_M_COFS
            lda         ZP_M_SZ1
            sta         SH_WINDOW,Y
            clc

@leave:
            _M_SYS_LEAVE                                    ; Restores the RAM bank and U
            PULL_YX
            jmp         MM_RETURN

@ref:                                                       ; A reference: through its far pointer (ROM,
            ldx         ZP_M_SZ1                            ;   FP_RO: ERR_MEM_NOT_SUPPORTED)
            ldy         ZP_M_COFS
            jsr         FP_WRITE
            bra         @leave

; Map a shared allocation's first bank into the $8000-$9FFF window (SH_WINDOW), for speed.  The calling
; task must hold a reference.  Undo with SH_UNLOCK.
; IN: .A = shared handle
; OUT (success): .X = previous RAM bank, .Y = previous U (pass both to SH_UNLOCK), C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID, C = 1 (.X, .Y preserved)
SH_LOCK:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            _M_SYS_ENTER                                    ; Select shared bank ID $00
            jsr         SH_HANDLE_PTR
            bcs         @fail
            jsr         SH_CHECK_REF
            bcs         @fail
            ldy         #ShHandle::count
            lda         (ZP_M_HP),Y
            cmp         #SH_REF_MARK
            bne         @allocation
            jsr         SH_REF_LOAD                         ; A reference: to shared RAM, its bank (its
            lda         ZP_FP + FarPtr::space               ;   data is at its address: SH_FP); to ROM,
            and         #FP_KIND                            ;   nothing to map (SH_READ it)
            cmp         #FP_SHARED
            bne         @unsupported
            lda         ZP_FP + FarPtr::sel
            bra         @map

@allocation:
            ldy         #ShHandle::bank
            lda         (ZP_M_HP),Y

@map:
            ply                                             ; Previous U        } Don't restore them:
            plx                                             ; Previous RAM bank } they're returned
            jsr         SH_SELECT_BANK                      ; Map the allocation
            pla                                             ; Drop the caller's .Y
            pla                                             ; Drop the caller's .X
            clc
            jmp         MM_RETURN

@unsupported:
            lda         #ERR_MEM_NOT_SUPPORTED
            sec

@fail:
            _M_SYS_LEAVE
            PULL_YX
            jmp         MM_RETURN

; Undo SH_LOCK.
; IN: .X = RAM bank, .Y = U (from SH_LOCK)
; OUT: C = 0
; Preserves .A, .X, .Y
SH_UNLOCK:
            stx         RAM_BANK_REG
            sty         U_REGISTER
            clc
            rts

; Drop all of a task's shared references, freeing banks nobody else references.  For MM_TASK_RESET.
; IN: .A = task
; Preserves .A, .X, .Y
SH_RESET_TASK:
            php                                             ; Save caller's I flag
            sei
            PUSH_AXY
            and         #$0F
            sta         ZP_M_TEMP
            _M_SYS_ENTER                                    ; Select shared bank ID $00
            lda         #1

@loop:
            sta         ZP_M_HANDLE
            jsr         SH_HANDLE_PTR
            bcs         @next                               ; Free entry
            jsr         SH_DROP_TASK_REF                    ; C = 1 if the task had no reference: fine

@next:
            lda         ZP_M_HANDLE
            inc
            bne         @loop                               ; Handles 1-255
            _M_SYS_LEAVE
            PULL_YXA
            plp                                             ; Restore caller's I flag
            rts

; ---- helpers (shared bank ID $00 selected, IRQs off)

; Point ZP_M_BM / ZP_M_BE at the shared bank maps.  Preserves .A
SH_MAPS_SETUP:
            pha
            LOAD_ADDR   SH_MAP, ZP_M_BM
            LOAD_ADDR   SH_ENDS, ZP_M_BE
            pla
            rts

; Find a free shared handle.  OUT: ZP_M_HANDLE, ZP_M_HP = its entry, C = 0; or .A = ERR_MEM_NO_HANDLES, C = 1
SH_NEW_HANDLE:
            lda         #1

@loop:
            sta         ZP_M_HANDLE
            jsr         SH_ENTRY_ADDR
            ldy         #ShHandle::count
            lda         (ZP_M_HP),Y
            beq         @found
            lda         ZP_M_HANDLE
            inc
            bne         @loop
            lda         #ERR_MEM_NO_HANDLES
            sec
            rts

@found:
            clc
            rts

; ZP_M_HP = entry of shared handle ZP_M_HANDLE: SH_HANDLES + (handle - 1) * 4.  Modifies: .A
SH_ENTRY_ADDR:
            lda         ZP_M_HANDLE
            dec
            stz         ZP_M_HP + 1
            asl
            rol         ZP_M_HP + 1
            asl
            rol         ZP_M_HP + 1
            clc
            adc         #<SH_HANDLES
            sta         ZP_M_HP
            lda         ZP_M_HP + 1
            adc         #>SH_HANDLES
            sta         ZP_M_HP + 1
            rts

; Point ZP_M_HP at a shared handle's entry and check it's allocated.
; IN: .A = shared handle.  OUT: C = 0; or .A = ERR_MEM_NOT_VALID, C = 1.  Modifies: .A, .Y
SH_HANDLE_PTR:
            sta         ZP_M_HANDLE
            cmp         #0
            beq         @bad
            jsr         SH_ENTRY_ADDR
            ldy         #ShHandle::count
            lda         (ZP_M_HP),Y
            beq         @bad
            clc
            rts

@bad:
            lda         #ERR_MEM_NOT_VALID
            sec
            rts

; Task ZP_M_TEMP's bit in an entry's mask.  OUT: .Y = ShHandle::mask_lo or mask_hi, .A = bit.  Modifies: .X
SH_TASK_BIT:
            lda         ZP_M_TEMP
            and         #7
            tax
            ldy         #ShHandle::mask_lo
            lda         ZP_M_TEMP
            and         #$08
            beq         :+
            iny                                             ; mask_hi
:
            lda         MMU_BIT_MASKS,X
            rts

; Give task ZP_M_TEMP a reference to entry ZP_M_HP.  Modifies: .A, .X, .Y
SH_SET_TASK_BIT:
            jsr         SH_TASK_BIT
            ora         (ZP_M_HP),Y
            sta         (ZP_M_HP),Y
            rts

; C = 0 if the calling task holds a reference to entry ZP_M_HP; else .A = ERR_MEM_NOT_VALID, C = 1.
; Modifies: .A, .X, .Y
SH_CHECK_REF:
            lda         T_REGISTER
            sta         ZP_M_TEMP
            jsr         SH_TASK_BIT
            and         (ZP_M_HP),Y
            beq         @no_ref
            clc
            rts

@no_ref:
            lda         #ERR_MEM_NOT_VALID
            sec
            rts

; Drop task ZP_M_TEMP's reference to entry ZP_M_HP; free the banks if it was the last reference.
; OUT: C = 0; or .A = ERR_MEM_NOT_VALID, C = 1 if the task had no reference.  Modifies: .A, .X, .Y
SH_DROP_TASK_REF:
            jsr         SH_TASK_BIT
            sta         ZP_M_SV
            and         (ZP_M_HP),Y
            beq         @no_ref
            lda         ZP_M_SV                             ; Clear the task's bit
            eor         #$FF
            and         (ZP_M_HP),Y
            sta         (ZP_M_HP),Y
            ldy         #ShHandle::mask_lo
            lda         (ZP_M_HP),Y
            iny
            ora         (ZP_M_HP),Y
            bne         @done                               ; Still referenced
            ldy         #ShHandle::count
            lda         (ZP_M_HP),Y
            cmp         #SH_REF_MARK
            beq         @free_entry                         ; A reference (SH_REF): no banks
            jsr         SH_MAPS_SETUP                       ; Last reference: free the banks
            ldy         #ShHandle::bank
            lda         (ZP_M_HP),Y
            ldx         #$FF
            jsr         BM_FREE_RUN

@free_entry:
            lda         #0
            ldy         #ShHandle::count                    ; Free the entry
            sta         (ZP_M_HP),Y

@done:
            clc
            rts

@no_ref:
            lda         #ERR_MEM_NOT_VALID
            sec
            rts

; Check the calling task's reference to shared handle .A and map its first bank; or, for a reference
; (SH_REF, fp.s), load its far pointer instead.
; OUT: C = 0 and V = 0 (mapped) or V = 1 (a reference: ZP_FP); or .A = ERR_MEM_NOT_VALID, C = 1
; Modifies: .A, .X, .Y
SH_ACCESS_SETUP:
            jsr         SH_HANDLE_PTR
            bcs         @done
            jsr         SH_CHECK_REF
            bcs         @done
            ldy         #ShHandle::count
            lda         (ZP_M_HP),Y
            cmp         #SH_REF_MARK
            beq         @ref
            ldy         #ShHandle::bank
            lda         (ZP_M_HP),Y
            jsr         SH_SELECT_BANK
            clv
            clc

@done:
            rts

@ref:
            jsr         SH_REF_LOAD                         ; ZP_FP (C = 0)
            bit         MMU_BIT_MASKS + 6                   ; V = 1 ($40)
            rts

; Map shared bank ID .A at $8000-$9FFF: U = ID >> 4, RAM bank = $F0 | (ID & $0F).  Preserves .X, .Y
SH_SELECT_BANK:
            pha
            and         #$0F
            ora         #$F0
            sta         RAM_BANK_REG
            pla
            lsr
            lsr
            lsr
            lsr
            sta         U_REGISTER
            rts
