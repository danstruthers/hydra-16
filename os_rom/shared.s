.debuginfo

.segment "SHARED"

; ****************************************************************************
; Shared memory (see MMU_PLAN.md)
;
;   256 shared 8K banks: shared bank ID = U << 4 | (bank & $0F), seen at $8000-$9FFF with RAM_BANK_REG =
;   $F0-$FF.  The system data lives in shared bank ID $00 (U = 0, bank $F0):
;       $8000-$81FF  message ring pointers (msg.s)
;       $8200-$821F  SH_MAP     shared bank bitmap (1 = in use)
;       $8220-$823F  SH_ENDS    run-end bitmap (last bank of each allocation)
;       $8400-$87FF  SH_HANDLES shared handle table: 255 entries of ShHandle
;       $8800-$88FF  IO_DEV_TABLE the IO device table (io.s)
;   Bank IDs $00 (system), $01-$08 (message rings) and $09 (IO transfer areas) are reserved, as are the
;   banks of any U macro-page
;   whose RAM isn't installed.
;
;   A shared handle is a 1-byte index (1-255) that any task can use, so it can be sent in a message.
;   Each task that uses it holds a reference (SH_ALLOC / SH_ATTACH); the banks are freed when the last
;   reference goes (SH_DETACH, or MM_TASK_RESET of the task).  Only tasks holding a reference can read or
;   write through the handle.  All calls: C = 0 on success, C = 1 with the error in .A.

SH_MAP              = $8200
SH_ENDS             = $8220
SH_HANDLES          = $8400
SH_MAX_HANDLES      = 255
SH_FIRST_FREE_ID    = $0A                                   ; Below: system data, message rings, IO transfers
SH_WINDOW           = PAGED_RAM_BASE                        ; Where SH_LOCK maps a shared allocation
SH_PROBE_ADDR       = $9FFF                                 ; Probe byte (bank $F0 of each U)
SH_PROBE_MARK       = $50                                   ; Probe marker: SH_PROBE_MARK + U

.struct     ShHandle
            bank        .byte                               ; First shared bank ID
            count       .byte                               ; Banks (0 = free entry)
            mask_lo     .byte                               ; Tasks $0-$7 holding a reference (bit = task)
            mask_hi     .byte                               ; Tasks $8-$F
.endstruct

; Set up the shared memory tables: find which U macro-pages have RAM, clear the bitmaps and handle
; table, and reserve the system and message ring banks.  Called by MMU_INIT at boot.
SHARED_RAM_INIT:
            php                                             ; Save caller's I flag
            sei
            PUSH_AXY
            _M_MSG_ENTER                                    ; Save RAM bank / U; select shared bank ID $00

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
            ldy         #MSG_SHARED_U
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
            ldx         #0                                  ; Reserve the banks of missing macro-pages

@macro_pages:
            lsr         ZP_TEMP_VEC + 1                     ; C = this U's present bit
            ror         ZP_TEMP_VEC
            bcs         @present
            lda         #$FF
            sta         SH_MAP,X
            sta         SH_MAP + 1,X

@present:
            inx
            inx
            cpx         #32
            bne         @macro_pages
            lda         #$FF                                ; Reserve bank IDs $00-$09
            ora         SH_MAP
            sta         SH_MAP
            lda         #$03
            ora         SH_MAP + 1
            sta         SH_MAP + 1
            _M_MSG_LEAVE
            PULL_YXA
            plp                                             ; Restore caller's I flag
            rts

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
            _M_MSG_ENTER                                    ; Select shared bank ID $00
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
            _M_MSG_LEAVE

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
            _M_MSG_ENTER                                    ; Select shared bank ID $00
            jsr         SH_HANDLE_PTR
            bcs         @leave
            lda         T_REGISTER
            sta         ZP_M_TEMP
            jsr         SH_SET_TASK_BIT
            clc

@leave:
            _M_MSG_LEAVE
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
            _M_MSG_ENTER                                    ; Select shared bank ID $00
            jsr         SH_HANDLE_PTR
            bcs         @leave
            lda         T_REGISTER
            sta         ZP_M_TEMP
            jsr         SH_DROP_TASK_REF

@leave:
            _M_MSG_LEAVE
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
            _M_MSG_ENTER                                    ; Select shared bank ID $00
            jsr         SH_ACCESS_SETUP                     ; Selects the allocation's first bank
            bcs         @leave
            ldy         ZP_M_COFS
            lda         SH_WINDOW,Y
            clc

@leave:
            _M_MSG_LEAVE                                    ; Restores the RAM bank and U
            PULL_YX
            jmp         MM_RETURN

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
            _M_MSG_ENTER                                    ; Select shared bank ID $00
            jsr         SH_ACCESS_SETUP                     ; Selects the allocation's first bank
            bcs         @leave
            ldy         ZP_M_COFS
            lda         ZP_M_SZ1
            sta         SH_WINDOW,Y
            clc

@leave:
            _M_MSG_LEAVE                                    ; Restores the RAM bank and U
            PULL_YX
            jmp         MM_RETURN

; Map a shared allocation's first bank into the $8000-$9FFF window (SH_WINDOW), for speed.  The calling
; task must hold a reference.  Undo with SH_UNLOCK.
; IN: .A = shared handle
; OUT (success): .X = previous RAM bank, .Y = previous U (pass both to SH_UNLOCK), C = 0
; OUT (failure): .A = ERR_MEM_NOT_VALID, C = 1 (.X, .Y preserved)
SH_LOCK:
            php                                             ; Save caller's I flag
            sei
            PUSH_XY
            _M_MSG_ENTER                                    ; Select shared bank ID $00
            jsr         SH_HANDLE_PTR
            bcs         @fail
            jsr         SH_CHECK_REF
            bcs         @fail
            ldy         #ShHandle::bank
            lda         (ZP_M_HP),Y
            ply                                             ; Previous U        } Don't restore them:
            plx                                             ; Previous RAM bank } they're returned
            jsr         SH_SELECT_BANK                      ; Map the allocation
            pla                                             ; Drop the caller's .Y
            pla                                             ; Drop the caller's .X
            clc
            jmp         MM_RETURN

@fail:
            _M_MSG_LEAVE
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
            _M_MSG_ENTER                                    ; Select shared bank ID $00
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
            _M_MSG_LEAVE
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
            jsr         SH_MAPS_SETUP                       ; Last reference: free the banks
            ldy         #ShHandle::bank
            lda         (ZP_M_HP),Y
            ldx         #$FF
            jsr         BM_FREE_RUN
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

; Check the calling task's reference to shared handle .A and map its first bank.
; OUT: C = 0; or .A = ERR_MEM_NOT_VALID, C = 1.  Modifies: .A, .X, .Y
SH_ACCESS_SETUP:
            jsr         SH_HANDLE_PTR
            bcs         @done
            jsr         SH_CHECK_REF
            bcs         @done
            ldy         #ShHandle::bank
            lda         (ZP_M_HP),Y
            jsr         SH_SELECT_BANK
            clc

@done:
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
