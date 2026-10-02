.debuginfo

; ****************************************************************************
; Per-task namespaces (see docs/plans/IO_PLAN.md, Phase 3, and NAMESPACES.md; the entry layout is in include/io.inc).
; BIOS ROM page 2, included inside `.scope PAGE2` after io.s (see all.s).  Each task's namespace is in its IO
; transfer area (IO_BLK_NS: 16 entries, two pages), after the data area IO_OPEN resolves names in, so
; everything here works with the IO transfer bank mapped (_M_IO_MAP_XFER).
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
            jsr         IO_XFER_REMAP
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
            jsr         IO_XFER_REMAP
            clc
            rts

@next_entry:
            lda         ZP_IO_CHUNK
            clc
            adc         #IO_DEV_SIZE
            sta         ZP_IO_CHUNK
            bne         @entry                      ; 16 entries: $8800-$88FF
            jsr         IO_XFER_REMAP
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

; Apply the namespace to the name in the data area (IO_OPEN).  The entries with the longest prefix that matches
; the name are a union's members (one, mostly), in the table's order: the one used is the first, or for a create,
; the first with NS_C (none: ERR_IO_MODE); or, for the first union a name meets, the one after the members already
; tried (IO_BLK_SKIP: IO_OPEN tries the next on ERR_IO_NOT_FOUND, NS_RETRY), with how many are left after it in
; IO_BLK_ULEFT.  A hide entry: ERR_IO_NOT_FOUND.
; OUT: C = 0: a mount: .A = its device, and the rest of the name is in the data area
;      C = 1: .A = 0: no entry matches (the name, maybe rewritten by binds, is in the data area);
;             or .A = ERR_IO_NS_LOOP, ERR_IO_NAME (a bind made the name too long), ERR_IO_NOT_FOUND (hidden, or
;             no members left) or ERR_IO_MODE (a create in a union with no NS_C member)
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
            jsr         NS_NAME_PTR
            stz         ZP_IO_TMP                   ; The longest prefix that matches (0: none)
            jsr         NS_FIRST

@longest:
            jsr         NS_MATCH
            bcs         :+
            cmp         ZP_IO_TMP
            bcc         :+
            sta         ZP_IO_TMP
:
            jsr         NS_NEXT
            bne         @longest
            lda         ZP_IO_TMP
            bne         :+
            sec                                     ; (.A = 0: no match)
            rts
:
            stz         ZP_IO_CNT                   ; Its members: ZP_IO_CNT of them
            jsr         NS_FIRST
:
            jsr         NS_MEMBER
            bne         :+
            inc         ZP_IO_CNT
:
            jsr         NS_NEXT
            bne         :--
            stz         ZP_IO_CNT + 1               ; Which: ZP_IO_CNT + 1 (from 0)
            lda         ZP_IO_CNT
            cmp         #2
            bcc         @pick                       ; (One: it)
            ldy         #IO_BLK_CALL
            lda         (ZP_IO_XFER),Y
            cmp         #H9_CREATE
            beq         @create
            ldy         #IO_BLK_ULEFT
            lda         (ZP_IO_XFER),Y
            bpl         @pick                       ; (A union met before, in this name: this one's first)
            ldy         #IO_BLK_SKIP
            lda         (ZP_IO_XFER),Y
            sta         ZP_IO_CNT + 1
            cmp         ZP_IO_CNT
            bcs         @hidden                     ; (No more members)
            lda         ZP_IO_CNT                   ; Left after it: the members - 1 - it
            clc
            sbc         ZP_IO_CNT + 1
            ldy         #IO_BLK_ULEFT
            sta         (ZP_IO_XFER),Y
            bra         @pick

@create:                                            ; A create: the first member with NS_C
            jsr         NS_FIRST
:
            jsr         NS_MEMBER
            bne         :+
            lda         (ZP_IO_CHUNK)               ; (NS_TYPE: NS_C is bit 7)
            bmi         @pick
            inc         ZP_IO_CNT + 1
:
            jsr         NS_NEXT
            bne         :--
            lda         #ERR_IO_MODE
            sec
            rts

@pick:                                              ; ZP_IO_CHUNK = member ZP_IO_CNT + 1
            jsr         NS_FIRST
:
            jsr         NS_MEMBER
            bne         :+
            lda         ZP_IO_CNT + 1
            beq         @apply
            dec         ZP_IO_CNT + 1
:
            jsr         NS_NEXT
            bne         :--

@hidden:
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

@apply:
            lda         (ZP_IO_CHUNK)               ; (NS_TYPE)
            and         #NS_KIND
            cmp         #NS_HIDE
            beq         @hidden
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

; ZP_IO_LEFT: (ZP_IO_LEFT),Y with .Y = NS_PREFIX + n = the data area's name's character n.  Modifies: .A, .X
NS_NAME_PTR:
            lda         #<(-NS_PREFIX)
            sta         ZP_IO_LEFT
            ldx         ZP_IO_DATA + 1
            dex
            stx         ZP_IO_LEFT + 1
            rts

; Does entry ZP_IO_CHUNK (one in use) match the name (ZP_IO_LEFT: NS_NAME_PTR): its prefix, then the name's
; end or a '/'?  OUT: C = 0, .A = the prefix's length; or C = 1.  Preserves .X
NS_MATCH:
            lda         (ZP_IO_CHUNK)               ; (NS_TYPE)
            beq         @no
            ldy         #NS_PREFIX

@compare:
            lda         (ZP_IO_CHUNK),Y
            beq         @end
            cmp         (ZP_IO_LEFT),Y
            bne         @no
            iny
            bra         @compare

@end:                                               ; The whole prefix matched: a whole path element?
            lda         (ZP_IO_LEFT),Y
            beq         @yes
            cmp         #'/'
            bne         @no

@yes:
            tya
            sec
            sbc         #NS_PREFIX
            clc
            rts

@no:
            sec
            rts

; Is entry ZP_IO_CHUNK one of the members NS_RESOLVE found: does it match, ZP_IO_TMP long?  OUT: Z = 1 if it
; is.  Preserves .X
NS_MEMBER:
            jsr         NS_MATCH
            bcs         :+
            cmp         ZP_IO_TMP
            rts
:
            lda         #1                          ; (Z = 0)
            rts

; IO_OPEN: the tidy name kept (IO_BLK_ORIG), for a union's next member; none skipped, none met yet.
; Modifies: .A, .Y, ZP_IO_LEFT
NS_ORIG_SAVE:
            ldy         #IO_BLK_SKIP
            lda         #0
            sta         (ZP_IO_XFER),Y
            jsr         NS_ORIG_PTR

@copy:
            lda         (ZP_IO_DATA),Y
            sta         (ZP_IO_LEFT),Y
            beq         @done
            iny
            cpy         #IO_ORIG_MAX
            bne         @copy

@done:
            rts

; ZP_IO_LEFT = IO_BLK_ORIG, .Y = 0; and no union met yet (IO_BLK_ULEFT).  Modifies: .A
NS_ORIG_PTR:
            ldy         #IO_BLK_ULEFT
            lda         #$FF
            sta         (ZP_IO_XFER),Y
            lda         ZP_IO_XFER                  ; (A page's start: no carry)
            ora         #IO_BLK_ORIG
            sta         ZP_IO_LEFT
            lda         ZP_IO_XFER + 1
            sta         ZP_IO_LEFT + 1
            ldy         #0
            rts

; IO_OPEN, after ERR_IO_NOT_FOUND: the next member of the first union the name met, if there's one.
; OUT: C = 0: one more member skipped, and the tidy name back in the data area; or C = 1: no more.
; Modifies: .A, .Y, ZP_IO_LEFT
NS_RETRY:
            ldy         #IO_BLK_ULEFT
            lda         (ZP_IO_XFER),Y
            beq         @none
            bmi         @none                       ; (No union met)
            ldy         #IO_BLK_SKIP
            lda         (ZP_IO_XFER),Y
            inc
            sta         (ZP_IO_XFER),Y
            jsr         NS_ORIG_PTR

@copy:
            lda         (ZP_IO_LEFT),Y
            sta         (ZP_IO_DATA),Y
            beq         @done
            iny
            bra         @copy

@done:
            clc
            rts

@none:
            sec
            rts

; (A name with no end within 256 bytes)
NS_NAME_BAD:
            lda         #ERR_IO_NAME
            sec
            rts

; Make the name in the data area absolute and tidy: a name that doesn't start with '/' is taken as relative
; to the task's current directory, then "." and ".." are worked out, and "//" and a '/' at the end taken out.
; OUT: C = 0; or C = 1, .A = ERR_IO_NAME (too long with the directory in front).
; Modifies: .X, .Y, ZP_IO_TMP, ZP_IO_CNT, ZP_IO_BYTE, ZP_IO_LEFT, ZP_IO_OFS
NS_ABS:
            lda         (ZP_IO_DATA)
            cmp         #'/'
            beq         NS_TIDY
            jsr         NS_CWD_PTR                          ; The directory's length: .Y
            ldy         #0
:
            lda         (ZP_IO_LEFT),Y
            beq         :+
            iny
            beq         NS_NAME_BAD                         ; (No end to it)
            bra         :-
:
            iny                                             ; The name moves up to make room for it and a '/'
            sty         ZP_IO_CNT
            stz         ZP_IO_TMP
            jsr         NS_SHIFT
            bcc         :+
            lda         #ERR_IO_NAME
            rts
:
            jsr         NS_CWD_PTR                          ; The directory, then '/'
            ldy         #0
:
            lda         (ZP_IO_LEFT),Y
            beq         :+
            sta         (ZP_IO_DATA),Y
            iny
            beq         NS_NAME_BAD                         ; (No end to it)
            bra         :-
:
            lda         #'/'
            sta         (ZP_IO_DATA),Y

; Tidy the absolute name in the data area (NS_ABS).  In place: what's written never passes what's read.
; ZP_IO_TMP = where it reads, ZP_IO_BYTE = where it writes, ZP_IO_OFS = the element's start, then its length.
; OUT: C = 0
NS_TIDY:
            stz         ZP_IO_TMP
            stz         ZP_IO_BYTE

@element:
            ldy         ZP_IO_TMP
:
            lda         (ZP_IO_DATA),Y                      ; ('/'s before it)
            cmp         #'/'
            bne         :+
            iny
            beq         NS_NAME_BAD                         ; (No end to it)
            bra         :-
:
            cmp         #0
            beq         @end
            sty         ZP_IO_OFS                           ; The element: to the next '/' or the end
:
            iny
            beq         NS_NAME_BAD                         ; (No end to it)
            lda         (ZP_IO_DATA),Y
            beq         :+
            cmp         #'/'
            bne         :-
:
            sty         ZP_IO_TMP
            tya
            sec
            sbc         ZP_IO_OFS
            sta         ZP_IO_OFS + 1                       ; (Its length)
            ldy         ZP_IO_OFS
            lda         (ZP_IO_DATA),Y
            cmp         #'.'
            bne         @keep
            lda         ZP_IO_OFS + 1
            cmp         #1
            beq         @element                            ; ".": nothing
            cmp         #2
            bne         @keep
            iny
            lda         (ZP_IO_DATA),Y
            cmp         #'.'
            bne         @keep
            ldy         ZP_IO_BYTE                          ; "..": back to the last '/' written
:
            cpy         #0
            beq         :+
            dey
            lda         (ZP_IO_DATA),Y
            cmp         #'/'
            bne         :-
:
            sty         ZP_IO_BYTE
            bra         @element

@keep:
            ldy         ZP_IO_BYTE                          ; '/', then the element
            lda         #'/'
            sta         (ZP_IO_DATA),Y
            inc         ZP_IO_BYTE
            ldx         ZP_IO_OFS + 1
:
            ldy         ZP_IO_OFS
            lda         (ZP_IO_DATA),Y
            inc         ZP_IO_OFS
            ldy         ZP_IO_BYTE
            sta         (ZP_IO_DATA),Y
            inc         ZP_IO_BYTE
            dex
            bne         :-
            bra         @element

@end:
            ldy         ZP_IO_BYTE                          ; Nothing left: the top, "/"
            bne         :+
            lda         #'/'
            sta         (ZP_IO_DATA),Y
            iny
:
            lda         #0
            sta         (ZP_IO_DATA),Y
            clc
            rts

; ZP_IO_LEFT = the task's current directory (the IO transfer bank mapped).  Modifies: .A
NS_CWD_PTR:
            lda         #IO_BLK_CWD
            sta         ZP_IO_LEFT
            lda         ZP_IO_XFER + 1
            sta         ZP_IO_LEFT + 1
            rts

; IO_CHDIR, with the tidy name in the data area: it's the task's current directory now, and the old one is
; kept at the data area + $80 (NS_CHDIR_UNDO puts it back if the new one turns out not to be a directory).
; OUT: C = 1, .A = ERR_IO_NAME (longer than 63 characters); or C = 0 and Z = 1: it's "/", with nothing to
; check; or C = 0 and Z = 0: open it to check it.  Modifies: .A, .Y, ZP_IO_LEFT, ZP_IO_OFS
NS_CHDIR_SET:
            ldy         #0
:
            lda         (ZP_IO_DATA),Y
            beq         :+
            iny
            bne         @far1                               ; (No end to it)
            jmp         NS_NAME_BAD
@far1:
            bra         :-
:
            cpy         #IO_CWD_MAX
            bcc         :+
            lda         #ERR_IO_NAME
            rts
:
            jsr         NS_CWD_PTR
            cpy         #1
            bne         @set
            lda         #0                                  ; "/": an empty directory name (Z = 1)
            sta         (ZP_IO_LEFT)
            clc
            rts

@set:
            jsr         NS_OLD_PTR
            ldy         #IO_CWD_MAX - 1                     ; The old one, kept ...
:
            lda         (ZP_IO_LEFT),Y
            sta         (ZP_IO_OFS),Y
            dey
            bpl         :-
            ldy         #0                                  ; ... and the new one
:
            lda         (ZP_IO_DATA),Y
            sta         (ZP_IO_LEFT),Y
            beq         :+
            iny
            bra         :-
:
            lda         #IO_CALL_CHDIR_SET                  ; (So a failure puts the old one back)
            ldy         #IO_BLK_CALL
            sta         (ZP_IO_XFER),Y                      ; (Z = 0)
            clc
            rts

; Put the old current directory back (NS_CHDIR_SET).  Modifies: .A, .Y, ZP_IO_LEFT, ZP_IO_OFS
NS_CHDIR_UNDO:
            jsr         NS_CWD_PTR
            jsr         NS_OLD_PTR
            ldy         #IO_CWD_MAX - 1
:
            lda         (ZP_IO_OFS),Y
            sta         (ZP_IO_LEFT),Y
            dey
            bpl         :-
            rts

; ZP_IO_OFS = where NS_CHDIR_SET keeps the old directory: the data area + $80.  Modifies: .A
NS_OLD_PTR:
            lda         #$80
            sta         ZP_IO_OFS
            lda         ZP_IO_DATA + 1
            sta         ZP_IO_OFS + 1
            rts

.assert     IO_CWD_MAX - 1 + NS_MAX_REWRITES * NS_TARGET_MAX < $80, error, "A name IO_CHDIR resolves stays below the old directory, at the data area + $80"

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
            stz         ZP_IO_CHUNK                 ; (IO_BLK_NS: a page's start)
            lda         ZP_IO_XFER + 1
            clc
            adc         #>IO_BLK_NS
            sta         ZP_IO_CHUNK + 1
            ldx         #NS_ENTRIES
            rts

; ZP_IO_CHUNK = the next entry; Z = 1 after the last one.  Modifies: .A, .X
NS_NEXT:
            lda         ZP_IO_CHUNK
            clc
            adc         #NS_ENTRY_SIZE
            sta         ZP_IO_CHUNK
            bcc         :+
            inc         ZP_IO_CHUNK + 1             ; (The namespace's second page)
:
            dex
            rts

; Clear every task's namespace (IO_INIT, at boot).  Modifies: .A, .X, .Y
NS_CLEAR_ALL:
            stz         ZP_IO_TMP                   ; (The task)

@task:
            lda         ZP_IO_TMP
            jsr         IO_XFER_OF                  ; (Its bank: .Y)
            sta         ZP_IO_XFER + 1
            stz         ZP_IO_XFER
            sty         ZP_IO_BUF
            _M_IO_MAP_XFER
            lda         ZP_IO_BUF
            sta         RAM_BANK_REG
            jsr         NS_CLEAR_MAPPED
            _M_IO_UNMAP
            inc         ZP_IO_TMP
            lda         ZP_IO_TMP
            cmp         #MAX_TASK_NUMBER + 1
            bne         @task
            rts

; Clear this task's namespace (IO_CLOSE_ALL, when the task ends).  Modifies: .A, .X, .Y
NS_CLEAR:
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            jsr         NS_CLEAR_MAPPED

NS_UNMAP_RTS:
            _M_IO_UNMAP
            rts

; (The IO transfer bank mapped; ZP_IO_XFER = the task's area.)  The current directory goes back to "/" too, and
; /proc/N/cmd's line and mark go
NS_CLEAR_MAPPED:
            lda         #0
            ldy         #IO_BLK_CWD
            sta         (ZP_IO_XFER),Y
            ldy         #IO_BLK_CMDST
            sta         (ZP_IO_XFER),Y
            iny
            sta         (ZP_IO_XFER),Y              ; (IO_BLK_CMD)
            jsr         NS_FIRST

@entry:
            lda         #NS_FREE
            sta         (ZP_IO_CHUNK)               ; (NS_TYPE)
            jsr         NS_NEXT
            bne         @entry
            rts

; Give task .A a copy of this task's namespace and current directory (IO_INHERIT).  Its area can be in the
; other transfer bank, so each byte is read with this task's bank mapped, and written with its.
; Modifies: .A, .X, .Y, ZP_IO_TMP, ZP_IO_CHUNK
NS_COPY_TO:
            jsr         IO_XFER_OF                  ; Its area, and its bank (ZP_IO_TMP)
            sta         ZP_IO_LEFT + 1
            stz         ZP_IO_LEFT
            sty         ZP_IO_TMP
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            lda         RAM_BANK_REG                ; This task's bank (ZP_IO_CHUNK)
            sta         ZP_IO_CHUNK
            ldy         #IO_BLK_CWD                 ; The current directory: to the request block page's end
            jsr         @bytes
            .assert     IO_BLK_CWD + IO_CWD_MAX = $100, error, "NS_COPY_TO: the current directory to $FF"
            lda         ZP_IO_XFER + 1              ; The namespace: its two pages
            pha
            clc
            adc         #>IO_BLK_NS
            sta         ZP_IO_XFER + 1
            lda         ZP_IO_LEFT + 1
            adc         #>IO_BLK_NS
            sta         ZP_IO_LEFT + 1
            ldy         #0
            jsr         @bytes
            inc         ZP_IO_XFER + 1
            inc         ZP_IO_LEFT + 1
            jsr         @bytes
            pla
            sta         ZP_IO_XFER + 1
            bra         NS_UNMAP_RTS

@bytes:                                             ; From .Y to the page's end
            ldx         ZP_IO_CHUNK
            stx         RAM_BANK_REG
            lda         (ZP_IO_XFER),Y
            ldx         ZP_IO_TMP
            stx         RAM_BANK_REG
            sta         (ZP_IO_LEFT),Y
            iny
            bne         @bytes
            ldx         ZP_IO_CHUNK
            stx         RAM_BANK_REG
            rts

; ****************************************************************************
; The calls

; The calls' start (after PUSH_XY): .X = the flags (ZP_IO_MODE), .A.Y = the path (ZP_IO_OFS), the transfer
; area mapped (_M_IO_UNMAP at the end), and the names copied in (NS_NAMES_IN: C = 1, the second one too).
; OUT: C = 0; or .A = ERR_IO_NAME, C = 1.  (Inline: IO_CALLER_PAGE looks at the call's own return)
.macro _M_NS_CALL_IN
            stx         ZP_IO_MODE
            sta         ZP_IO_OFS
            sty         ZP_IO_OFS + 1
            lda         #0                          ; (C, kept)
            rol
            sta         ZP_IO_BYTE
            jsr         IO_CALLER_PAGE              ; .X = the caller's ROM page
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            lsr         ZP_IO_BYTE
            jsr         NS_NAMES_IN
.endmacro

; Attach a device's server at a path in this task's namespace: names under the path go to it, with the
; rest of the name.  .X's flags (0: the path's entries are replaced): NS_BEFORE, NS_AFTER, a member of a union
; before or after the path's others; NS_CREATE, a create in the union goes to it (NS_ADD).
; IN: .A.Y = the path ("/...", 13 characters at most), ZP_IO_BUF = the device's name (e.g. "zero"), .X = flags
; OUT: C = 0; or .A = ERR_IO_NAME, ERR_IO_NOT_FOUND (no such device) or ERR_IO_NS_FULL, C = 1
; (The names are read as the caller sees them: NS_NAMES_IN.)
IO_MOUNT:
            PUSH_XY
            sec                                     ; (Both names)
            _M_NS_CALL_IN
            bcs         @done
            lda         ZP_IO_BUF
            sta         ZP_IO_LEFT
            lda         ZP_IO_BUF + 1
            sta         ZP_IO_LEFT + 1
            jsr         IO_DEV_FIND                 ; .A = the device
            bcs         @not_found
            pha
            jsr         NS_ADD                      ; ZP_IO_CHUNK = its entry
            ply
            bcs         @done
            tya
            ldy         #NS_DEV
            sta         (ZP_IO_CHUNK),Y
            lda         #NS_MOUNT
            jsr         NS_SET_TYPE
            bra         @done

@not_found:
            lda         #ERR_IO_NOT_FOUND

@done:
            _M_IO_UNMAP
            PULL_YX
            rts

; Make a path stand for another in this task's namespace: IO_OPEN of a name under the path opens the same
; name under the target.  .X's flags: as IO_MOUNT's; or NS_HIDDEN: hide the path (no target, ZP_IO_BUF not
; read): nothing under it is found, in this task and the tasks it starts.
; IN: .A.Y = the path ("/...", 13 characters at most), ZP_IO_BUF = the target ("/...", 15 at most), .X = flags
; OUT: C = 0; or .A = ERR_IO_NAME or ERR_IO_NS_FULL, C = 1
; (The names are read as the caller sees them: NS_NAMES_IN.)
IO_BIND:
            PUSH_XY
            pha                                     ; (C = 1: the target too; a hide: just the path)
            txa
            and         #NS_HIDDEN
            eor         #NS_HIDDEN
            cmp         #1
            pla
            _M_NS_CALL_IN
            bcs         @done
            lda         ZP_IO_MODE
            and         #NS_HIDDEN
            beq         @bind
            jsr         NS_ADD                      ; A hide
            bcs         @done
            lda         #NS_HIDE
            bra         @type

@bind:
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
            jsr         NS_ADD                      ; ZP_IO_CHUNK = its entry
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

@type:
            jsr         NS_SET_TYPE
            bra         @done

@bad_name:
            lda         #ERR_IO_NAME
            sec

@done:
            _M_IO_UNMAP
            PULL_YX
            rts

; Remove a path's entries from this task's namespace: all of them (.X = 0), or one member of its union: the
; bind to ZP_IO_BUF, or the mount of the device ZP_IO_BUF names (.X <> 0).
; IN: .A.Y = the path, .X, ZP_IO_BUF.  OUT: C = 0; or .A = ERR_IO_NOT_FOUND or ERR_IO_NAME, C = 1
IO_UNMOUNT:
            PUSH_XY
            cpx         #1                          ; (C = 1: the member's name too)
            _M_NS_CALL_IN
            bcs         @done
            lda         ZP_IO_MODE
            beq         @all
            lda         ZP_IO_BUF                   ; The device the member names ($FF: none)
            sta         ZP_IO_LEFT
            lda         ZP_IO_BUF + 1
            sta         ZP_IO_LEFT + 1
            jsr         IO_DEV_FIND
            bcc         :+
            lda         #$FF
:
            sta         ZP_IO_TMP
            lda         ZP_IO_BUF                   ; ZP_IO_BUF: (ZP_IO_BUF),Y with .Y = NS_TARGET + n = the
            sec                                     ;   member's character n
            sbc         #NS_TARGET
            sta         ZP_IO_BUF
            bcs         :+
            dec         ZP_IO_BUF + 1
:
            jsr         NS_PATH_PTR
            ldx         #0

@member:
            txa
            jsr         NS_AT
            jsr         NS_SAME
            bne         @next
            lda         (ZP_IO_CHUNK)               ; (NS_TYPE)
            and         #NS_KIND
            cmp         #NS_MOUNT
            bne         @bind
            ldy         #NS_DEV
            lda         (ZP_IO_CHUNK),Y
            cmp         ZP_IO_TMP
            beq         @drop
            bra         @next

@bind:
            cmp         #NS_BIND
            bne         @next
            ldy         #NS_TARGET
:
            lda         (ZP_IO_CHUNK),Y
            cmp         (ZP_IO_BUF),Y
            bne         @next
            iny
            cmp         #0
            bne         :-

@drop:
            txa
            jsr         NS_DELETE
            clc
            bra         @done

@next:
            inx
            cpx         #NS_ENTRIES
            bne         @member
            lda         #ERR_IO_NOT_FOUND
            sec
            bra         @done

@all:
            jsr         NS_PATH_PTR
            jsr         NS_DROP_ALL

@done:
            _M_IO_UNMAP
            PULL_YX
            rts

; Copy a call's names into the IO data area (the IO transfer bank mapped), reading them as the calling
; code sees them (far pointers: RAM, the paged ROM, or its own ROM page .X): the path (ZP_IO_OFS) to the
; data area's start and, if C = 1, ZP_IO_BUF's name to its middle; ZP_IO_OFS and ZP_IO_BUF then point at
; the copies.  OUT: C = 0; or .A = ERR_IO_NAME (unreadable, or 128 bytes with no end), C = 1.
; Modifies: .A, .Y
NS_NAMES_IN:
            php                                     ; (C: the second name too)
            lda         ZP_IO_OFS
            ldy         ZP_IO_OFS + 1
            jsr         FP_MAKE                     ; The path
            lda         ZP_IO_DATA
            sta         ZP_IO_OFS
            ldy         ZP_IO_DATA + 1
            sty         ZP_IO_OFS + 1
            jsr         NS_NAME_COPY
            bcs         @fail
            plp
            bcc         @done
            lda         ZP_IO_BUF
            ldy         ZP_IO_BUF + 1
            jsr         FP_MAKE                     ; The second name
            lda         ZP_IO_DATA
            ora         #$80
            sta         ZP_IO_BUF
            ldy         ZP_IO_DATA + 1
            sty         ZP_IO_BUF + 1
            jmp         NS_NAME_COPY

@fail:
            plp
            sec

@done:
            rts

; ZP_FP's string -> .A.Y: 128 bytes at most, with its 0.  OUT: C = 0; or .A = ERR_IO_NAME, C = 1.
; Preserves .X
NS_NAME_COPY:
            phx
            ldx         #$80
            sec
            jsr         FP_COPY
            plx
            bcc         @done
            lda         #ERR_IO_NAME

@done:
            rts

; The entry for a mount, bind or hide at the path (ZP_IO_OFS: NS_NAMES_IN's), with the call's flags (ZP_IO_MODE).
; The table has no gaps (the entries in use come first, in order), and a union's members are next to each other:
; with neither NS_BEFORE nor NS_AFTER, the path's entries go, and it's added after the rest; NS_BEFORE, before the
; path's first member; NS_AFTER, after its last (none: after the rest).
; OUT: ZP_IO_CHUNK = it, its prefix filled in (the caller sets NS_TYPE: NS_SET_TYPE), C = 0; or .A = ERR_IO_NAME
; or ERR_IO_NS_FULL, C = 1.  Modifies: .A, .X, .Y, ZP_IO_LEFT, ZP_IO_TMP, ZP_IO_CNT
NS_ADD:
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
            jsr         NS_PATH_PTR
            lda         ZP_IO_MODE
            and         #NS_BEFORE | NS_AFTER
            bne         :+
            jsr         NS_DROP_ALL                 ; (Replaced: the path's entries go)
:
            jsr         NS_USED
            cmp         #NS_ENTRIES
            bcc         :+
            lda         #ERR_IO_NS_FULL
            sec
            rts
:
            sta         ZP_IO_TMP                   ; Where: after the rest, unless the path has members
            ldx         #0

@member:
            txa
            jsr         NS_AT
            jsr         NS_SAME
            bne         @next
            lda         ZP_IO_MODE
            and         #NS_BEFORE
            beq         :+
            stx         ZP_IO_TMP                   ; (Before its first)
            bra         @place
:
            inx                                     ; (After it, so far)
            stx         ZP_IO_TMP
            dex

@next:
            inx
            cpx         #NS_ENTRIES
            bne         @member

@place:
            lda         ZP_IO_TMP
            jsr         NS_GAP
            lda         ZP_IO_TMP
            jsr         NS_AT
            ldy         #NS_PREFIX

@prefix:
            lda         (ZP_IO_LEFT),Y
            sta         (ZP_IO_CHUNK),Y
            beq         :+
            iny
            bra         @prefix
:
            clc
            rts

; NS_TYPE of entry ZP_IO_CHUNK = .A (NS_MOUNT, NS_BIND, NS_HIDE), with NS_C if the call's flags (ZP_IO_MODE) have
; NS_CREATE.  OUT: C = 0.  Modifies: .A
NS_SET_TYPE:
            sta         ZP_IO_TMP
            lda         ZP_IO_MODE
            and         #NS_CREATE
            beq         :+
            lda         #NS_C
:
            ora         ZP_IO_TMP
            sta         (ZP_IO_CHUNK)
            clc
            rts

; ZP_IO_LEFT: (ZP_IO_LEFT),Y with .Y = NS_PREFIX + n = the path's (ZP_IO_OFS) character n.  Modifies: .A
NS_PATH_PTR:
            lda         ZP_IO_OFS
            sec
            sbc         #NS_PREFIX
            sta         ZP_IO_LEFT
            lda         ZP_IO_OFS + 1
            sbc         #0
            sta         ZP_IO_LEFT + 1
            rts

; Is entry ZP_IO_CHUNK (one in use) the path's (ZP_IO_LEFT: NS_PATH_PTR), its prefix the same?  OUT: Z = 1 if it
; is.  Modifies: .A, .Y.  Preserves .X
NS_SAME:
            lda         (ZP_IO_CHUNK)               ; (NS_TYPE)
            beq         @no
            ldy         #NS_PREFIX

@compare:
            lda         (ZP_IO_LEFT),Y
            cmp         (ZP_IO_CHUNK),Y
            bne         @done
            cmp         #0
            beq         @done                       ; (Both ended: Z = 1)
            iny
            bra         @compare

@no:
            lda         #1                          ; (Z = 0)

@done:
            rts

; Remove every entry for the path (ZP_IO_LEFT: NS_PATH_PTR).  OUT: C = 0 if there was one; or .A =
; ERR_IO_NOT_FOUND, C = 1.  Modifies: .A, .X, .Y, ZP_IO_TMP, ZP_IO_CNT
NS_DROP_ALL:
            stz         ZP_IO_TMP                   ; (Any?)
            ldx         #0

@entry:
            txa
            jsr         NS_AT
            jsr         NS_SAME
            bne         @next
            phx
            txa
            jsr         NS_DELETE                   ; (The next is at .X now)
            plx
            inc         ZP_IO_TMP
            bra         @entry

@next:
            inx
            cpx         #NS_ENTRIES
            bne         @entry
            lda         ZP_IO_TMP
            bne         :+
            lda         #ERR_IO_NOT_FOUND
            sec
            rts
:
            clc
            rts

; ZP_IO_CHUNK = entry .A.  Preserves .A, .X, .Y
NS_AT:
            pha
            asl                                     ; (.A * 32: 8 to a page)
            asl
            asl
            asl
            asl
            sta         ZP_IO_CHUNK
            pla
            pha
            lsr
            lsr
            lsr
            clc
            adc         ZP_IO_XFER + 1
            adc         #>IO_BLK_NS
            sta         ZP_IO_CHUNK + 1
            pla
            rts

; .A = the entries in use: the first ones (the table has no gaps).  Modifies: .X
NS_USED:
            jsr         NS_FIRST
:
            lda         (ZP_IO_CHUNK)               ; (NS_TYPE)
            beq         :+
            jsr         NS_NEXT
            bne         :-
:
            txa                                     ; NS_ENTRIES - the ones not looked at
            eor         #$FF
            sec
            adc         #NS_ENTRIES
            rts

; Remove entry .A: the ones after it move down one, and the last in use is free then.  Modifies: .A, .X, .Y,
; ZP_IO_CNT
NS_DELETE:
            tax

@next:
            inx
            cpx         #NS_ENTRIES
            beq         @last
            txa
            jsr         NS_AT
            lda         (ZP_IO_CHUNK)               ; (Free: the one before was the last in use)
            beq         @last
            lda         ZP_IO_CHUNK                 ; It, to the entry before
            sec
            sbc         #NS_ENTRY_SIZE
            sta         ZP_IO_CNT
            lda         ZP_IO_CHUNK + 1
            sbc         #0
            sta         ZP_IO_CNT + 1
            jsr         NS_MOVE
            bra         @next

@last:
            dex
            txa
            jsr         NS_AT
            lda         #NS_FREE
            sta         (ZP_IO_CHUNK)               ; (NS_TYPE)
            rts

; Make room at entry .A: the ones in use from it on move up one (there's a free one after them).
; Modifies: .A, .X, .Y, ZP_IO_CNT
NS_GAP:
            pha
            jsr         NS_USED
            tax                                     ; .X = the next to move + 1
            pla
            sta         ZP_IO_CNT                   ; (Kept while .X counts down to it)

@move:
            cpx         ZP_IO_CNT
            beq         @done
            dex
            phx
            lda         ZP_IO_CNT
            pha
            txa
            jsr         NS_AT
            lda         ZP_IO_CHUNK                 ; It, to the entry after
            clc
            adc         #NS_ENTRY_SIZE
            sta         ZP_IO_CNT
            lda         ZP_IO_CHUNK + 1
            adc         #0
            sta         ZP_IO_CNT + 1
            jsr         NS_MOVE
            pla
            sta         ZP_IO_CNT
            plx
            bra         @move

@done:
            rts

; Copy entry ZP_IO_CHUNK to ZP_IO_CNT.  Modifies: .A, .Y.  Preserves .X
NS_MOVE:
            ldy         #NS_ENTRY_SIZE - 1
:
            lda         (ZP_IO_CHUNK),Y
            sta         (ZP_IO_CNT),Y
            dey
            bpl         :-
            rts
