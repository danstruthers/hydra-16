.debuginfo

; ****************************************************************************
; Per-task namespaces (see IO_PLAN.md, Phase 3; the entry layout is in include/io.inc).  BIOS ROM page 2,
; included inside `.scope PAGE2` after io.s (see all.s).  Each task's namespace is in its IO transfer
; area (IO_BLK_NS), next to the data area IO_OPEN resolves names in, so everything here works with the
; IO transfer bank mapped (_M_IO_MAP_XFER).
;
;   IO_MOUNT "/sd", "sd"          names under /sd go to the device sd (its server gets the rest: /x/y)
;   IO_BIND "/null", "/dev/null"  names under /null stand for names under /dev/null
;   IO_UNMOUNT "/sd"              removes the entry for that path (mount or bind)
; A prefix matches a whole path element: /sd matches /sd and /sd/x, not /sdx.  The longest match wins.
; New tasks get a copy of their parent's namespace (IO_INHERIT); a task's is cleared when it ends.

.segment "IO_P2"

; Find a device by name.  IN: ZP_IO_LEFT = the name (it ends at '/' or 0; readable with the IO
; transfer bank mapped: the data area, task RAM or this ROM page)
; OUT: .A = the device (device table index), .Y = the name's length, C = 0; or C = 1 (not there)
; The IO transfer bank is mapped (again) on return.  Modifies: .X, ZP_IO_CHUNK, ZP_IO_TMP
IO_DEV_FIND:
            lda         #<IO_DEV_TABLE
            sta         ZP_IO_CHUNK                 ; ZP_IO_CHUNK = the entry
            lda         #>IO_DEV_TABLE
            sta         ZP_IO_CHUNK + 1

@entry:
            ldx         #SYS_BANK               ; The device table (shared bank ID $00; U is 0)
            stx         RAM_BANK_REG
            lda         (ZP_IO_CHUNK)
            beq         @next_entry                 ; Free entry
            ldy         #0

@compare:
            ldx         #IO_XFER_BANK
            stx         RAM_BANK_REG
            lda         (ZP_IO_LEFT),Y              ; The name's character; '/' or 0 ends it
            cmp         #'/'
            bne         :+
            lda         #0
:
            sta         ZP_IO_TMP
            ldx         #SYS_BANK
            stx         RAM_BANK_REG
            cpy         #IO_DEV_NAME_LEN
            beq         @all_8                      ; 8 characters matched
            cmp         (ZP_IO_CHUNK),Y
            bne         @next_entry
            iny
            lda         ZP_IO_TMP
            bne         @compare                    ; Both ended: a match
            dey                                     ; .Y = the name's length
            bra         @found

@all_8:
            lda         ZP_IO_TMP
            bne         @next_entry                 ; The name is longer than 8

@found:
            lda         ZP_IO_CHUNK                 ; Device index = (entry - table) / 16
            sec
            sbc         #<IO_DEV_TABLE
            lsr
            lsr
            lsr
            lsr
            ldx         #IO_XFER_BANK
            stx         RAM_BANK_REG
            clc
            rts

@next_entry:
            lda         ZP_IO_CHUNK
            clc
            adc         #IO_DEV_SIZE
            sta         ZP_IO_CHUNK
            bne         @entry                      ; 16 entries: $8800-$88FF
            ldx         #IO_XFER_BANK
            stx         RAM_BANK_REG
            sec
            rts

; Remove the first .A characters of the name in the data area.  Modifies: .A, .Y
NS_CUT:
            sta         ZP_IO_LEFT
            lda         ZP_IO_DATA + 1
            sta         ZP_IO_LEFT + 1
            ldy         #0

@copy:
            lda         (ZP_IO_LEFT),Y
            sta         (ZP_IO_DATA),Y
            beq         @done
            iny
            bne         @copy

@done:
            rts

; Apply the namespace to the name in the data area (IO_OPEN).
; OUT: C = 0: a mount: .A = its device, and the rest of the name is in the data area
;      C = 1: .A = 0: no entry matches (the name, maybe rewritten by binds, is in the data area);
;             or .A = ERR_IO_NS_LOOP or ERR_IO_NAME (a bind made the name too long)
; Modifies: .X, .Y, ZP_IO_BUF, ZP_IO_CHUNK, ZP_IO_CNT, ZP_IO_TMP, ZP_IO_BYTE, ZP_IO_LEFT, ZP_IO_OFS
NS_RESOLVE:
            lda         #NS_MAX_REWRITES + 1
            sta         ZP_IO_BYTE

@again:
            dec         ZP_IO_BYTE
            bne         :+
            lda         #ERR_IO_NS_LOOP
            sec
            rts
:
            stz         ZP_IO_TMP                   ; The longest prefix so far (0: none)
            lda         #<(-NS_PREFIX)              ; ZP_IO_LEFT: (ZP_IO_LEFT),Y with .Y = NS_PREFIX + n
            sta         ZP_IO_LEFT                  ;   = the name's character n
            ldx         ZP_IO_DATA + 1
            dex
            stx         ZP_IO_LEFT + 1
            jsr         NS_FIRST                    ; ZP_IO_CHUNK = the first entry, .X = count

@entry:
            lda         (ZP_IO_CHUNK)               ; (NS_TYPE)
            beq         @next
            ldy         #NS_PREFIX

@compare:
            lda         (ZP_IO_CHUNK),Y
            beq         @prefix_end
            cmp         (ZP_IO_LEFT),Y
            bne         @next
            iny
            bra         @compare

@prefix_end:                                        ; The whole prefix matched: a whole path element?
            lda         (ZP_IO_LEFT),Y
            beq         @match
            cmp         #'/'
            bne         @next

@match:
            tya
            sec
            sbc         #NS_PREFIX                  ; Its length
            cmp         ZP_IO_TMP
            bcc         @next                       ; (Shorter than the best so far)
            beq         @next
            sta         ZP_IO_TMP
            lda         ZP_IO_CHUNK
            sta         ZP_IO_BUF                   ; ZP_IO_BUF = the best entry (its low byte: they're
                                                    ;   all in the same page)
@next:
            jsr         NS_NEXT
            bne         @entry
            lda         ZP_IO_TMP
            bne         @apply
            sec                                     ; (.A = 0: no match)
            rts

@apply:
            lda         ZP_IO_BUF
            sta         ZP_IO_CHUNK
            lda         (ZP_IO_CHUNK)               ; (NS_TYPE)
            cmp         #NS_MOUNT
            bne         @bind
            lda         ZP_IO_TMP
            jsr         NS_CUT                      ; The rest of the name, for the server
            ldy         #NS_DEV
            lda         (ZP_IO_CHUNK),Y
            clc
            rts

@bind:                                              ; The name = the target, then the rest of the name
            ldy         #NS_TARGET
:
            lda         (ZP_IO_CHUNK),Y
            beq         :+
            iny
            bra         :-
:
            tya
            sec
            sbc         #NS_TARGET
            sta         ZP_IO_CNT                   ; The target's length
            jsr         NS_SHIFT                    ; The rest: from offset ZP_IO_TMP to ZP_IO_CNT
            bcs         @too_long
            lda         #<(-NS_TARGET)              ; (ZP_IO_LEFT),Y with .Y = NS_TARGET + n = the
            sta         ZP_IO_LEFT                  ;   name's character n
            ldx         ZP_IO_DATA + 1
            dex
            stx         ZP_IO_LEFT + 1
            ldy         #NS_TARGET

@target:
            lda         (ZP_IO_CHUNK),Y
            beq         @rewritten
            sta         (ZP_IO_LEFT),Y
            iny
            bra         @target

@rewritten:
            jmp         @again                      ; (The new name can match an entry too)

@too_long:
            lda         #ERR_IO_NAME
            rts

; Move the end of the name in the data area, from offset ZP_IO_TMP to offset ZP_IO_CNT (the terminating
; 0 too).  OUT: C = 0; or C = 1 if it wouldn't fit (nothing moved).  Modifies: .A, .Y, ZP_IO_LEFT,
; ZP_IO_OFS
NS_SHIFT:
            ldy         ZP_IO_TMP                   ; .Y = the name's end (its 0)
:
            lda         (ZP_IO_DATA),Y
            beq         :+
            iny
            bra         :-
:
            tya
            sec
            sbc         ZP_IO_TMP                   ; .A = the bytes to move - 1
            clc
            adc         ZP_IO_CNT
            bcs         @done                       ; (Past the end of the data area: C = 1)
            lda         ZP_IO_TMP                   ; ZP_IO_LEFT = from, ZP_IO_OFS = to
            sta         ZP_IO_LEFT
            lda         ZP_IO_CNT
            sta         ZP_IO_OFS
            lda         ZP_IO_DATA + 1
            sta         ZP_IO_LEFT + 1
            sta         ZP_IO_OFS + 1
            tya
            sec
            sbc         ZP_IO_TMP
            tay                                     ; .Y = the bytes to move - 1
            lda         ZP_IO_CNT
            cmp         ZP_IO_TMP
            beq         @same
            bcs         @up

@down:                                              ; To a lower offset: first byte first
            ldy         #$FF
:
            iny
            lda         (ZP_IO_LEFT),Y
            sta         (ZP_IO_OFS),Y
            bne         :-                          ; (Up to and including the 0)
            clc
            rts

@up:                                                ; To a higher offset: last byte first
            lda         (ZP_IO_LEFT),Y
            sta         (ZP_IO_OFS),Y
            dey
            cpy         #$FF
            bne         @up

@same:
            clc

@done:
            rts

; ZP_IO_CHUNK = this task's first namespace entry, .X = NS_ENTRIES.  Modifies: .A
NS_FIRST:
            lda         #IO_BLK_NS
            sta         ZP_IO_CHUNK
            lda         ZP_IO_XFER + 1
            sta         ZP_IO_CHUNK + 1
            ldx         #NS_ENTRIES
            rts

; ZP_IO_CHUNK = the next entry; Z = 1 after the last one.  Modifies: .A, .X
NS_NEXT:
            lda         ZP_IO_CHUNK
            clc
            adc         #NS_ENTRY_SIZE
            sta         ZP_IO_CHUNK
            dex
            rts

; Clear every task's namespace (IO_INIT, at boot).  Modifies: .A, .X, .Y
NS_CLEAR_ALL:
            stz         ZP_IO_XFER
            lda         #>PAGED_RAM_BASE
            sta         ZP_IO_XFER + 1
            _M_IO_MAP_XFER

@task:
            jsr         NS_CLEAR_MAPPED
            inc         ZP_IO_XFER + 1              ; The next task's area: + $200
            inc         ZP_IO_XFER + 1
            lda         ZP_IO_XFER + 1
            cmp         #>(PAGED_RAM_BASE + 16 * $200)
            bne         @task
            bra         NS_UNMAP_RTS

; Clear this task's namespace (IO_CLOSE_ALL, when the task ends).  Modifies: .A, .X, .Y
NS_CLEAR:
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            jsr         NS_CLEAR_MAPPED

NS_UNMAP_RTS:
            _M_IO_UNMAP
            rts

; (The IO transfer bank mapped; ZP_IO_XFER = the task's area)
NS_CLEAR_MAPPED:
            jsr         NS_FIRST

@entry:
            lda         #NS_FREE
            sta         (ZP_IO_CHUNK)               ; (NS_TYPE)
            jsr         NS_NEXT
            bne         @entry
            rts

; Give task .A a copy of this task's namespace (IO_INHERIT).  Modifies: .A, .X, .Y
NS_COPY_TO:
            asl                                     ; Its area: $8000 + task * $200
            ora         #>PAGED_RAM_BASE
            sta         ZP_IO_LEFT + 1
            stz         ZP_IO_LEFT
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            ldy         #IO_BLK_NS

@copy:
            lda         (ZP_IO_XFER),Y
            sta         (ZP_IO_LEFT),Y
            iny
            bne         @copy                       ; (To the end of the request block page)
            .assert     IO_BLK_NS + NS_ENTRIES * NS_ENTRY_SIZE = $100, error, "NS_COPY_TO copies to $FF"
            bra         NS_UNMAP_RTS

; ****************************************************************************
; The calls

; Attach a device's server at a path in this task's namespace: names under the path go to it, with the
; rest of the name.  An entry for the same path is replaced.
; IN: .A.Y = the path ("/...", 13 characters at most), ZP_IO_BUF = the device's name (e.g. "zero")
; OUT: C = 0; or .A = ERR_IO_NAME, ERR_IO_NOT_FOUND (no such device) or ERR_IO_NS_FULL, C = 1
; (The names must not be in the BIOS ROM: see IO_NAME_CHECK.)
IO_MOUNT:
            jsr         IO_NAMES_CHECK
            bcc         @ok
            rts

@ok:
            PUSH_XY
            sta         ZP_IO_OFS                   ; ZP_IO_OFS = the path
            sty         ZP_IO_OFS + 1
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            lda         ZP_IO_BUF
            sta         ZP_IO_LEFT
            lda         ZP_IO_BUF + 1
            sta         ZP_IO_LEFT + 1
            jsr         IO_DEV_FIND                 ; .A = the device
            bcs         @not_found
            pha
            jsr         NS_ENTRY_FOR                ; ZP_IO_CHUNK = the path's entry
            ply
            bcs         @done
            tya
            ldy         #NS_DEV
            sta         (ZP_IO_CHUNK),Y
            lda         #NS_MOUNT
            sta         (ZP_IO_CHUNK)               ; (NS_TYPE)
            clc
            bra         @done

@not_found:
            lda         #ERR_IO_NOT_FOUND

@done:
            _M_IO_UNMAP
            PULL_YX
            rts

; Make a path stand for another in this task's namespace: IO_OPEN of a name under the path opens the same
; name under the target.  An entry for the same path is replaced.
; IN: .A.Y = the path ("/...", 13 characters at most), ZP_IO_BUF = the target ("/...", 15 at most)
; OUT: C = 0; or .A = ERR_IO_NAME or ERR_IO_NS_FULL, C = 1
; (The names must not be in the BIOS ROM: see IO_NAME_CHECK.)
IO_BIND:
            jsr         IO_NAMES_CHECK
            bcc         @ok
            rts

@ok:
            PUSH_XY
            sta         ZP_IO_OFS                   ; ZP_IO_OFS = the path
            sty         ZP_IO_OFS + 1
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            lda         (ZP_IO_BUF)                 ; The target: "/...", short enough?
            cmp         #'/'
            bne         @bad_name
            ldy         #0
:
            lda         (ZP_IO_BUF),Y
            beq         :+
            iny
            cpy         #NS_TARGET_MAX + 1
            bne         :-
            bra         @bad_name
:
            jsr         NS_ENTRY_FOR                ; ZP_IO_CHUNK = the path's entry
            bcs         @done
            lda         #NS_TARGET                  ; ZP_IO_LEFT: (ZP_IO_LEFT),Y with .Y = NS_TARGET + n =
            eor         #$FF                        ;   the target's character n
            sec
            adc         ZP_IO_BUF
            sta         ZP_IO_LEFT
            lda         ZP_IO_BUF + 1
            sbc         #0
            sta         ZP_IO_LEFT + 1
            ldy         #NS_TARGET

@copy:
            lda         (ZP_IO_LEFT),Y
            sta         (ZP_IO_CHUNK),Y
            beq         @copied
            iny
            bra         @copy

@copied:
            lda         #NS_BIND
            sta         (ZP_IO_CHUNK)               ; (NS_TYPE)
            clc
            bra         @done

@bad_name:
            lda         #ERR_IO_NAME
            sec

@done:
            _M_IO_UNMAP
            PULL_YX
            rts

; Remove the entry for a path (a mount or a bind) from this task's namespace.
; IN: .A.Y = the path.  OUT: C = 0; or .A = ERR_IO_NOT_FOUND or ERR_IO_NAME (in the BIOS ROM), C = 1
IO_UNMOUNT:
            jsr         IO_NAME_CHECK
            bcc         @ok
            rts

@ok:
            PUSH_XY
            sta         ZP_IO_OFS                   ; ZP_IO_OFS = the path
            sty         ZP_IO_OFS + 1
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            jsr         NS_FIND                     ; ZP_IO_CHUNK = its entry
            bcs         @done
            lda         #NS_FREE
            sta         (ZP_IO_CHUNK)               ; (NS_TYPE)
            clc

@done:
            _M_IO_UNMAP
            PULL_YX
            rts

; The entry for the path at ZP_IO_OFS: the existing one, or a free one with the path filled in (and
; NS_TYPE = NS_FREE until the caller sets it).  OUT: ZP_IO_CHUNK = the entry, C = 0; or .A = ERR_IO_NAME
; or ERR_IO_NS_FULL, C = 1.  Modifies: .A, .X, .Y, ZP_IO_LEFT
NS_ENTRY_FOR:
            lda         (ZP_IO_OFS)                 ; "/...", short enough?
            cmp         #'/'
            bne         @bad_name
            ldy         #0
:
            lda         (ZP_IO_OFS),Y
            beq         :+
            iny
            cpy         #NS_PREFIX_MAX + 1
            bne         :-

@bad_name:
            lda         #ERR_IO_NAME
            sec
            rts
:
            jsr         NS_FIND
            bcc         @done                       ; It's there: replace it
            jsr         NS_FIRST                    ; A free one

@free:
            lda         (ZP_IO_CHUNK)               ; (NS_TYPE)
            beq         @fill
            jsr         NS_NEXT
            bne         @free
            lda         #ERR_IO_NS_FULL
            sec
            rts

@fill:
            ldy         #NS_PREFIX

@copy:
            lda         (ZP_IO_LEFT),Y              ; (NS_FIND left ZP_IO_LEFT set up for the path)
            sta         (ZP_IO_CHUNK),Y
            beq         @done
            iny
            bra         @copy

@done:
            clc
            rts

; Print this task's namespace, an entry a line: "/path -> device" (a mount) or "/path = /target" (a bind).
; (Printing uses the IO transfer area too, so each byte is read on its own: NS_GETC.)
IO_NS_LIST:
            PUSH_AXY
            ldx         #IO_BLK_NS                  ; .X = the entry's offset

@entry:
            ldy         #NS_TYPE
            jsr         NS_GETC
            beq         @next
            pha
            ldy         #NS_PREFIX
            jsr         NS_PUTS
            pla
            cmp         #NS_MOUNT
            bne         @bind
            PRINT_CHAR  #' ', #'-', #'>', #' '
            ldy         #NS_DEV
            jsr         NS_GETC                     ; .A = the device
            jsr         NS_PUT_DEV
            bra         @eol

@bind:
            PRINT_CHAR  #' ', #'=', #' '
            ldy         #NS_TARGET
            jsr         NS_PUTS

@eol:
            PRINT_CRLF

@next:
            txa
            clc
            adc         #NS_ENTRY_SIZE
            tax
            bne         @entry                      ; (The last entry ends at $FF)
            PULL_YXA
            clc
            rts

; Print a string of the entry at offset .X, from its byte .Y.  Modifies: .A, .Y
NS_PUTS:
            jsr         NS_GETC
            beq         @done
            PRINT_CHAR
            iny
            bra         NS_PUTS

@done:
            rts

; Byte .Y of the entry at offset .X in this task's namespace.  OUT: .A (and Z).  Preserves .X, .Y
NS_GETC:
            sty         ZP_IO_TMP
            jsr         IO_XFER_SETUP
            stx         ZP_IO_CHUNK
            lda         ZP_IO_XFER + 1
            sta         ZP_IO_CHUNK + 1
            _M_IO_MAP_XFER
            ldy         ZP_IO_TMP
            lda         (ZP_IO_CHUNK),Y
            _M_IO_UNMAP
            ldy         ZP_IO_TMP
            ora         #0
            rts

; Print device .A's name (from the device table, in shared bank ID $00).  Preserves .X
NS_PUT_DEV:
            asl                                     ; Its entry: IO_DEV_TABLE + device * 16
            asl
            asl
            asl
            sta         ZP_IO_OFS                   ; (Printing changes ZP_IO_CHUNK, not ZP_IO_OFS)
            ldy         #0

@char:
            lda         ZP_IO_OFS
            sta         ZP_IO_CHUNK
            lda         #>IO_DEV_TABLE
            sta         ZP_IO_CHUNK + 1
            phy
            lda         RAM_BANK_REG                ; Map the device table (U = 0, bank $F0)
            pha
            lda         U_REGISTER
            pha
            stz         U_REGISTER
            lda         #SYS_BANK
            sta         RAM_BANK_REG
            lda         (ZP_IO_CHUNK),Y
            ply                                     ; (Back: pull without touching .A)
            sty         U_REGISTER
            ply
            sty         RAM_BANK_REG
            ply
            cmp         #0
            beq         @done
            PRINT_CHAR
            iny
            cpy         #IO_DEV_NAME_LEN
            bne         @char

@done:
            rts

; Find the entry for the path at ZP_IO_OFS (an exact match).  OUT: ZP_IO_CHUNK = the entry, C = 0; or
; .A = ERR_IO_NOT_FOUND, C = 1.  ZP_IO_LEFT is set so that (ZP_IO_LEFT),Y with .Y = NS_PREFIX + n is the
; path's character n.  Modifies: .A, .X, .Y
NS_FIND:
            lda         #NS_PREFIX
            eor         #$FF
            sec
            adc         ZP_IO_OFS
            sta         ZP_IO_LEFT
            lda         ZP_IO_OFS + 1
            sbc         #0
            sta         ZP_IO_LEFT + 1
            jsr         NS_FIRST

@entry:
            lda         (ZP_IO_CHUNK)               ; (NS_TYPE)
            beq         @next
            ldy         #NS_PREFIX

@compare:
            lda         (ZP_IO_LEFT),Y
            cmp         (ZP_IO_CHUNK),Y
            bne         @next
            cmp         #0                          ; Both ended: the same
            beq         @found
            iny
            bra         @compare

@found:
            clc
            rts

@next:
            jsr         NS_NEXT
            bne         @entry
            lda         #ERR_IO_NOT_FOUND
            sec
            rts
