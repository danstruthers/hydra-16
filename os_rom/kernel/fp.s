.debuginfo

; ****************************************************************************
; Far pointers and references.  BIOS ROM page 5, included inside `.scope PAGE5` (see all.s).
;
;   A plain address means different memory depending on what's mapped: T ($0000-$7FFF), the RAM bank
;   and U ($8000-$9FFF), the paged ROM bank ($A000-$DFFF), W ($E000-$FFFF).  A far pointer (FarPtr, in
;   kernel.inc) is the address and what's mapped there, so it reads the same from any ROM page and any
;   task: a string in ROM on page 4, a table in the paged ROM, a buffer in shared RAM.  (A task's own
;   RAM is the exception: only that task can read it through a far pointer.)
;
;   FP_MAKE     .A.Y = an address as the caller sees it, .X = the caller's ROM page -> ZP_FP
;   FP_READ     the byte at ZP_FP + .Y;  FP_WRITE  .X -> ZP_FP + .Y (RAM, not FP_RO)
;   FP_COPY     ZP_FP -> the caller's memory: .X bytes, or a string up to its 0
;
;   References: handles for far pointers, used like allocations' handles.
;   MM_REF      ZP_FP -> an MMU handle (this task's): MM_READ, MM_WRITE, MM_LOCK / MM_UNLOCK (task RAM
;               and the paged ROM), MM_FREE (drops the handle; the memory isn't the MMU's), MM_FP
;   SH_REF      ZP_FP (ROM or shared RAM) -> a shared handle, which any task can use (SH_ATTACH, SH_READ,
;               SH_WRITE, SH_LOCK for shared RAM, SH_DETACH, SH_FP); the calling task holds a reference
;   MM_FP, SH_FP  any handle (an allocation or a reference) -> ZP_FP, e.g. to FP_COPY from it
;
;   All calls: C = 0 on success, C = 1 with the error in .A.  The mappings are switched with IRQs off,
;   and put back before returning.

.segment "FP_P5"

; ---- far pointers

; Make a far pointer from an address, as the calling code sees it: task RAM ($0000-$7FFF: this task's),
; the RAM bank at $8000-$9FFF (this task's bank, or a shared bank), the paged ROM bank ($A000-$DFFF), or
; ROM page .X ($E000-$FFFF).  Code on a ROM page passes its page (W_REGISTER, or an API its caller's).
; IN: .A.Y = address, .X = ROM page.  OUT: ZP_FP, C = 0.  Preserves .A, .X, .Y
FP_MAKE:
            php
            sei
            pha
            sta         ZP_FP
            sty         ZP_FP + 1
            cpy         #>PAGED_RAM_BASE
            bcc         @task                               ; $0000-$7FFF
            cpy         #>(PAGED_RAM_BASE + $2000)
            bcc         @window                             ; $8000-$9FFF
            cpy         #>BIOS_ROM_START
            bcc         @prom                               ; $A000-$DFFF
            stx         ZP_FP + FarPtr::sel                 ; $E000-$FFFF: ROM page .X
            lda         #FP_BIOS | FP_RO
            bra         @space

@prom:
            lda         ROM_BANK_REG
            sta         ZP_FP + FarPtr::sel
            lda         #FP_PROM | FP_RO
            bra         @space

@window:
            lda         RAM_BANK_REG
            cmp         #SYS_BANK                           ; $F0-$FF: a shared bank
            bcc         @task
            and         #$0F
            sta         ZP_FP + FarPtr::sel
            lda         U_REGISTER                          ; Shared bank ID = U << 4 | bank & $0F
            asl
            asl
            asl
            asl
            ora         ZP_FP + FarPtr::sel
            sta         ZP_FP + FarPtr::sel
            lda         #FP_SHARED
            bra         @space

@task:
            lda         RAM_BANK_REG                        ; (For $8000-$9FFF: this task's RAM bank)
            sta         ZP_FP + FarPtr::sel
            lda         T_REGISTER                          ; FP_TASK, and whose RAM it is
            asl
            asl
            asl
            asl

@space:
            sta         ZP_FP + FarPtr::space
            pla
            plp
            clc
            rts

; Read a byte through ZP_FP.  IN: .Y = offset.  OUT: .A = the byte, C = 0; or .A = ERR_MEM_NOT_VALID
; (another task's RAM), C = 1.  Preserves .X, .Y
FP_READ:
            php
            sei
            jsr         FP_CHECK
            bcs         FP_FAIL
            jsr         FP_GET
            bra         FP_OK

; Write a byte through ZP_FP.  IN: .Y = offset, .X = the byte.  OUT: C = 0; or .A = ERR_MEM_NOT_VALID
; (another task's RAM) or ERR_MEM_NOT_SUPPORTED (ROM, or FP_RO), C = 1.  Preserves .X, .Y
FP_WRITE:
            php
            sei
            jsr         FP_CHECK
            bcs         FP_FAIL
            lda         ZP_FP + FarPtr::space
            and         #FP_ROM | FP_RO
            bne         @read_only
            jsr         FP_MAP
            txa
            sta         (ZP_FP),Y
            jsr         FP_UNMAP
            bra         FP_OK

@read_only:
            lda         #ERR_MEM_NOT_SUPPORTED

FP_FAIL:                                                    ; (The caller's P on the stack)
            sec

; Return with the caller's I flag (its P on the stack), and C
FP_RETURN:
            bcs         :+

FP_OK:
            plp
            clc
            rts
:
            plp
            sec
            rts

; Copy through ZP_FP into memory the caller sees at .A.Y (task RAM, or its RAM bank at $8000-$9FFF):
; .X bytes (0 = 256); or, with C = 1, a string: up to and including its 0 (at most .X bytes).
; OUT: .X = bytes copied (0 = 256), C = 0; or .A = ERR_MEM_NOT_VALID (another task's RAM) or
; ERR_MEM_BAD_ARG (a string with no 0 in .X bytes), C = 1.  Preserves .Y
FP_COPY:
            sta         ZP_FP_DST
            sty         ZP_FP_DST + 1
            stx         ZP_FP_N
            lda         #0
            rol
            sta         ZP_FP_MODE                          ; 1: a string
            php
            sei
            phy
            jsr         FP_CHECK
            bcs         @fail
            ldy         #0

@byte:
            jsr         FP_GET                              ; (Maps and puts back per byte: the destination can
            sta         (ZP_FP_DST),Y                       ;   be in the $8000 window too)
            iny
            ldx         ZP_FP_MODE
            beq         @count
            cmp         #0
            beq         @copied                             ; The string's 0

@count:
            cpy         ZP_FP_N
            beq         @end                                ; (ZP_FP_N = 0: .Y comes round to 0 after 256)
            tsx                                             ; A moment for IRQs between the bytes (FP_GET puts
            lda         $0102,X                             ;   back what it maps), if the caller had them on:
            and         #$04                                ;   a name copy mustn't hold off a serial byte
            bne         @byte                               ;   (the caller's P, under the .Y)
            cli
            nop
            sei
            bra         @byte

@end:
            lda         ZP_FP_MODE
            beq         @copied
            lda         #ERR_MEM_BAD_ARG                    ; A string that doesn't end in time

@fail:
            ply
            bra         FP_FAIL

@copied:
            tya
            tax                                             ; .X = bytes copied
            ply
            bra         FP_OK

; ---- helpers (IRQs off)

; C = 1, .A = ERR_MEM_NOT_VALID if ZP_FP is another task's RAM.  Preserves .X, .Y
FP_CHECK:
            lda         ZP_FP + FarPtr::space
            bit         #FP_KIND
            bne         @ok                                 ; Not FP_TASK
            lsr
            lsr
            lsr
            lsr
            eor         T_REGISTER
            bne         @bad

@ok:
            clc
            rts

@bad:
            lda         #ERR_MEM_NOT_VALID
            sec
            rts

.assert     FP_TASK = 0, error, "FP_CHECK: FP_TASK is kind 0"

; .A = the byte at ZP_FP + .Y (checked already).  Preserves .X, .Y
FP_GET:
            jsr         FP_MAP
            cmp         #FP_BIOS
            beq         @bios
            lda         (ZP_FP),Y
            bra         FP_UNMAP

@bios:
            jsr         FP_PEEK_PAGE                        ; (COMMON: switches W for the read)
            bra         FP_UNMAP

; Map what ZP_FP points into: its RAM bank (FP_TASK), shared bank (FP_SHARED: RAM bank and U) or paged
; ROM bank (FP_PROM).  (FP_BIOS: nothing; FP_PEEK_PAGE switches W.)  The ones it replaces go in ZP_FP_SAVE.
; OUT: .A = the kind.  Preserves .X, .Y
FP_MAP:
            lda         RAM_BANK_REG
            sta         ZP_FP_SAVE
            lda         ROM_BANK_REG
            sta         ZP_FP_SAVE + 1
            lda         U_REGISTER
            sta         ZP_FP_SAVE + 2
            lda         ZP_FP + FarPtr::space
            and         #FP_KIND
            beq         @task
            cmp         #FP_SHARED
            beq         @shared
            cmp         #FP_PROM
            bne         @done                               ; FP_BIOS
            lda         ZP_FP + FarPtr::sel
            sta         ROM_BANK_REG
            lda         #FP_PROM
            rts

@shared:
            lda         ZP_FP + FarPtr::sel
            lsr
            lsr
            lsr
            lsr
            sta         U_REGISTER
            lda         ZP_FP + FarPtr::sel
            ora         #SYS_BANK                           ; $F0 | ID & $0F
            sta         RAM_BANK_REG
            lda         #FP_SHARED
            rts

@task:
            lda         ZP_FP + FarPtr::sel
            sta         RAM_BANK_REG
            lda         #FP_TASK

@done:
            rts

; Put back what FP_MAP replaced.  Preserves .A, .X, .Y and the flags
FP_UNMAP:
            php
            pha
            lda         ZP_FP_SAVE
            sta         RAM_BANK_REG
            lda         ZP_FP_SAVE + 1
            sta         ROM_BANK_REG
            lda         ZP_FP_SAVE + 2
            sta         U_REGISTER
            pla
            plp
            rts

; ---- references: MMU handles (this task's)

; A handle for a far pointer, used like an allocation's handle (see the notes at the top).
; IN: ZP_FP.  OUT: .A = the handle, C = 0; or .A = ERR_MEM_NOT_VALID (another task's RAM) or
; ERR_MEM_NO_HANDLES, C = 1.  Preserves .X, .Y
MM_REF:
            php
            sei
            PUSH_XY
            jsr         FP_CHECK
            bcs         @done
            jsr         MM_NEW_HANDLE                       ; ZP_M_HP = the entry, ZP_M_HANDLE
            bcs         @done
            ldy         #Handle::addr_l
            lda         ZP_FP
            sta         (ZP_M_HP),Y
            iny
            lda         ZP_FP + 1
            sta         (ZP_M_HP),Y
            iny                                             ; Handle::bank
            lda         ZP_FP + FarPtr::sel
            sta         (ZP_M_HP),Y
            lda         ZP_FP + FarPtr::space
            and         #FP_RO
            beq         :+
            lda         #AI_READONLY
:
            ora         #AI_IN_USE | AI_REF
            sta         ZP_M_TEMP
            lda         ZP_FP + FarPtr::space
            and         #AI_REF_KIND
            ora         ZP_M_TEMP
            ldy         #Handle::status                     ; Status last: it makes the entry live
            sta         (ZP_M_HP),Y
            lda         ZP_M_HANDLE
            clc

@done:
            PULL_YX
            jmp         FP_RETURN

; The far pointer of an MMU handle: an allocation (task RAM, or its RAM bank) or a reference.
; IN: .A = handle.  OUT: ZP_FP, C = 0; or .A = ERR_MEM_NOT_VALID, C = 1.  Preserves .X, .Y
MM_FP:
            php
            sei
            phy
            jsr         MM_HANDLE_PTR                       ; .A = status
            bcs         @done
            jsr         MM_ENTRY_FP

@done:
            ply
            jmp         FP_RETURN

; ZP_FP = the far pointer of MMU handle entry ZP_M_HP.  IN: .A = its status.  OUT: C = 0.  Preserves .A, .X, .Y
MM_ENTRY_FP:
            phy
            pha
            ldy         #Handle::addr_l
            lda         (ZP_M_HP),Y
            sta         ZP_FP
            iny
            lda         (ZP_M_HP),Y
            sta         ZP_FP + 1
            iny                                             ; Handle::bank: the RAM bank (AI_PAGED), a
            lda         (ZP_M_HP),Y                         ;   reference's selector
            sta         ZP_FP + FarPtr::sel
            lda         T_REGISTER                          ; FP_TASK, this task's
            asl
            asl
            asl
            asl
            sta         ZP_FP + FarPtr::space
            pla
            pha
            and         #AI_REF
            cmp         #AI_REF
            bne         @allocation
            pla                                             ; A reference: its kind
            pha
            and         #AI_REF_KIND
            tsb         ZP_FP + FarPtr::space
            bra         @read_only

@allocation:
            pla
            pha
            bit         #AI_SMALL
            beq         @read_only
            lda         ZP_M_HP                             ; AI_SMALL: the bytes are in the entry itself
            sta         ZP_FP
            lda         ZP_M_HP + 1
            sta         ZP_FP + 1

@read_only:
            pla
            pha
            and         #AI_READONLY
            beq         :+
            lda         #FP_RO
            tsb         ZP_FP + FarPtr::space
:
            pla
            ply
            clc
            rts

; MM_LOCK of a reference (MM_LOCK: IRQs off, ZP_M_HP = the entry).  Task RAM: its RAM bank is mapped (for
; $8000-$9FFF); the paged ROM: its bank is ($01).  Shared RAM and the BIOS ROM can't be: use MM_READ,
; or MM_FP and FP_COPY.
; IN: .A = the entry's status.  OUT: .A.Y = pointer, .X = the bank it replaced (for MM_UNLOCK: the RAM
; bank, or for the paged ROM the paged ROM bank), C = 0; or .A = ERR_MEM_NOT_SUPPORTED, C = 1
MM_REF_LOCK:
            pha
            and         #AI_REF_KIND
            cmp         #FP_PROM
            beq         @prom
            cmp         #FP_TASK
            bne         @no
            ldx         RAM_BANK_REG                        ; (Returned either way)
            ldy         #Handle::addr_h
            lda         (ZP_M_HP),Y
            cmp         #>PAGED_RAM_BASE
            bcc         @lock                               ; $0000-$7FFF: nothing to map
            ldy         #Handle::bank
            lda         (ZP_M_HP),Y
            sta         RAM_BANK_REG
            bra         @lock

@prom:
            ldx         ROM_BANK_REG
            ldy         #Handle::bank
            lda         (ZP_M_HP),Y
            sta         ROM_BANK_REG

@lock:
            pla
            ora         #AI_LOCKED
            ldy         #Handle::status
            sta         (ZP_M_HP),Y
            ldy         #Handle::addr_l
            lda         (ZP_M_HP),Y
            pha
            iny
            lda         (ZP_M_HP),Y
            tay
            pla
            clc
            rts

@no:
            pla
            lda         #ERR_MEM_NOT_SUPPORTED
            sec
            rts

; ---- references: shared handles (any task's)

; A shared handle for a far pointer to ROM or shared RAM (not task RAM: only its task could read it),
; which any task can use (see the notes at the top).  The calling task holds the first reference.
; IN: ZP_FP.  OUT: .A = the shared handle, C = 0; or .A = ERR_MEM_BAD_ARG (task RAM) or
; ERR_MEM_NO_HANDLES, C = 1.  Preserves .X, .Y
SH_REF:
            php
            sei
            PUSH_XY
            lda         ZP_FP + FarPtr::space
            and         #FP_KIND
            bne         :+
            lda         #ERR_MEM_BAD_ARG                    ; FP_TASK
            sec
            bra         @done
:
            _M_SYS_ENTER                                    ; Select shared bank ID $00 (the handle table)
            jsr         SH_NEW_HANDLE                       ; ZP_M_HANDLE, ZP_M_HP = the entry
            bcs         @leave
            lda         #SH_REF_MARK                        ; A reference: its far pointer is in SH_REF_TBL
            ldy         #ShHandle::count
            sta         (ZP_M_HP),Y
            lda         #0
            ldy         #ShHandle::mask_lo
            sta         (ZP_M_HP),Y
            iny
            sta         (ZP_M_HP),Y
            jsr         SH_REF_ENTRY                        ; ZP_M_CP = its far pointer's place
            ldy         #.sizeof(FarPtr) - 1

@copy:
            lda         ZP_FP,Y
            sta         (ZP_M_CP),Y
            dey
            bpl         @copy
            lda         T_REGISTER
            sta         ZP_M_TEMP
            jsr         SH_SET_TASK_BIT                     ; The calling task's reference
            lda         ZP_M_HANDLE
            clc

@leave:
            _M_SYS_LEAVE

@done:
            PULL_YX
            jmp         FP_RETURN

; The far pointer of a shared handle the calling task holds: an allocation (its first bank, at $8000)
; or a reference.  IN: .A = shared handle.  OUT: ZP_FP, C = 0; or .A = ERR_MEM_NOT_VALID, C = 1.
; Preserves .X, .Y
SH_FP:
            php
            sei
            PUSH_XY
            _M_SYS_ENTER                                    ; Select shared bank ID $00
            jsr         SH_HANDLE_PTR
            bcs         @leave
            jsr         SH_CHECK_REF
            bcs         @leave
            ldy         #ShHandle::count
            lda         (ZP_M_HP),Y
            cmp         #SH_REF_MARK
            bne         @allocation
            jsr         SH_REF_LOAD
            bra         @leave

@allocation:
            stz         ZP_FP
            lda         #>PAGED_RAM_BASE
            sta         ZP_FP + 1
            lda         #FP_SHARED
            sta         ZP_FP + FarPtr::space
            ldy         #ShHandle::bank
            lda         (ZP_M_HP),Y
            sta         ZP_FP + FarPtr::sel
            clc

@leave:
            _M_SYS_LEAVE
            PULL_YX
            jmp         FP_RETURN

; ZP_FP = shared reference ZP_M_HANDLE's far pointer (shared bank ID $00 selected).  OUT: C = 0.
; Preserves .X; modifies .A, .Y
SH_REF_LOAD:
            jsr         SH_REF_ENTRY
            ldy         #.sizeof(FarPtr) - 1

@copy:
            lda         (ZP_M_CP),Y
            sta         ZP_FP,Y
            dey
            bpl         @copy
            clc
            rts

; ZP_M_CP = the place of shared reference ZP_M_HANDLE's far pointer: SH_REF_TBL + (handle - 1) * 4
; Modifies: .A
SH_REF_ENTRY:
            lda         ZP_M_HANDLE
            dec
            stz         ZP_M_CP + 1
            asl
            rol         ZP_M_CP + 1
            asl
            rol         ZP_M_CP + 1
            clc
            adc         #<SH_REF_TBL
            sta         ZP_M_CP
            lda         ZP_M_CP + 1
            adc         #>SH_REF_TBL
            sta         ZP_M_CP + 1
            rts

.assert     .sizeof(FarPtr) = 4, error, "SH_REF_ENTRY: 4-byte far pointers"
