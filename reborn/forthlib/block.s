; ****************************************************************************
; block.s - HyForth's Block word set (/lib/forth/block.fl: lib block): block, buffer, update, save-buffers, flush,
; load, blk; and its extension's empty-buffers, list, scr, thru (\ and refill in a block: the core's and Core
; Extension's, by BLK, the source record's src_blk).  Block u is the 1024 bytes at u * 1024 of the block file
; (Gforth's way): blocks.fb in the directory current as it's first wanted, made if it isn't there, or the one
; open-blocks names.  Two buffers: block reads into the one not given last (written first, if it was UPDATEd), and
; past the file's end a block is spaces.  Behind them, the task's RAM banks (taken as they're wanted, BC_BANKS at
; most, 8 blocks a bank) keep the blocks read and written: a block read again comes from its bank (a copy, not the
; file); an UPDATEd buffer goes to its bank, changed there, and save-buffers and flush write the changed ones to the
; file, as does the program's end (the core's call, blk_vec's 1).  With the banks all used, the oldest taken goes
; (to the file first, if it's changed); with none to be had, the file is the buffers' alone, as it was.  load
; interprets a block as the source (SOURCE its 1024 characters, BLK its number), and the core asks this library for a
; block source's buffer again when a source nested in it ends (blk_vec's 0: bk_src).  list shows a block's 16 lines
; of 64 (scr its number).  A file's failure: the system's ior, THROWn.

.include "forthlib.inc"

BLK_SIZE    = 1024
BC_BANKS    = 16                                            ; The banks behind the buffers, at most ...
BC_SLOTS    = BC_BANKS * 8                                  ;   and their blocks (a slot each, 8 a bank)
BC_HAS      = $01                                           ; A slot's state: a block's ...
BC_DIRTY    = $80                                           ;   changed since the file had it

.bss
bc_bank:    .res        BC_BANKS                            ; The banks (bc_nbank of them, taken as they're wanted)
bc_nbank:   .res        1
bc_none:    .res        1                                   ; <> 0: no bank to be had (the file alone)
bc_nlo:     .res        BC_SLOTS                            ; Each slot's block ...
bc_nhi:     .res        BC_SLOTS
bc_flag:    .res        BC_SLOTS                            ;   and its state (0: free)
bc_next:    .res        1                                   ; The slot taken next with them all in use (round)
bk_bufi:    .res        1                                   ; (A buffer, and a slot, a moment)
bk_sloti:   .res        1
bk_fd:      .res        1                                   ; The block file's fd ($FF: not open) ...
bk_name:    .res        64                                  ;   its name (counted; none: blocks.fb)
bk_cur:     .res        1                                   ; The buffer given last (0, 1) ...
bk_used:    .res        2                                   ;   each one's: <> 0, a block's ...
bk_upd:     .res        2                                   ;   <> 0, UPDATEd ...
bk_nlo:     .res        2                                   ;   its block ...
bk_nhi:     .res        2
bk_buf:     .res        BLK_SIZE * 2                        ;   and its bytes
bk_scr:     .res        2                                   ; SCR
bk_left:    .res        2                                   ; A read's or write's bytes left
.code

            HEADER      "block", 0
block:                                                      ; ( u -- a-addr ): its buffer, read in if it isn't in one
            jsr         bk_find
            bcc         bk_give
            jsr         bk_victim
            jsr         bk_read
            bra         bk_give

            HEADER      "buffer", 0
buffer:                                                     ; ( u -- a-addr ): a buffer for it, as it is (not read)
            jsr         bk_find
            bcc         bk_give
            jsr         bk_victim
; ( u -- a-addr ): buffer .Y (bk_find's, bk_victim's) given, block u's
bk_give:
            sty         bk_cur
            lda         #1
            sta         bk_used,y
            lda         dlo,x
            sta         bk_nlo,y
            lda         dhi,x
            sta         bk_nhi,y
            jsr         bk_addr
            lda         w
            sta         dlo,x
            lda         w + 1
            sta         dhi,x
            rts

            HEADER      "update", 0
update:                                                     ; The buffer given last: written before it's used again
            ldy         bk_cur
            lda         bk_used,y
            sta         bk_upd,y
            rts

            HEADER      "save-buffers", 0
savebuffers:                                                ; The UPDATEd ones written, and the banks' changed
            ldy         #0                                  ;   blocks to the file
            jsr         bk_save
            ldy         #1
            jsr         bk_save
            jmp         bc_flush

            HEADER      "flush", 0
flush_w:                                                    ; Them written, and the buffers free
            jsr         savebuffers
            jmp         emptybuffers

            HEADER      "empty-buffers", 0
emptybuffers:                                               ; The buffers free (none written)
            stz         bk_used
            stz         bk_used + 1
            stz         bk_upd
            stz         bk_upd + 1
            rts

            HEADER      "blk", 0
blk:                                                        ; ( -- a-addr ): the block being interpreted (0: none)
            CONSTCODE   src_blk

            HEADER      "scr", 0
scr:                                                        ; ( -- a-addr ): the block LIST showed last
            CONSTCODE   bk_scr

            HEADER      "load", 0
load:                                                       ; ( i*x u -- j*x ): block u interpreted, then the
            lda         dlo,x                               ;   source before (0: THROW -35)
            ora         dhi,x
            bne         :+
            lda         #<-35
            jmp         throw_a
:
            jsr         src_push
            lda         dlo,x
            sta         src_blk
            lda         dhi,x
            sta         src_blk + 1
            inx
            lda         #$FF
            sta         src_id
            sta         src_id + 1
            stz         src_close
            stz         to_in
            stz         to_in + 1
            stz         src_line
            stz         src_line + 1
            jsr         bk_src
            jsr         interpret
            jmp         src_pop

            HEADER      "thru", 0
thru:                                                       ; ( i*x u1 u2 -- j*x ): blocks u1 to u2 LOADed
            lda         dhi,x                               ; (u2 on the 6502's stack, its low byte on top)
            pha
            lda         dlo,x
            pha
            inx
@blk:
            stx         xsave                               ; Past u2: the end
            tsx
            lda         $0101,x
            ldy         $0102,x
            ldx         xsave
            sec
            sbc         dlo,x
            tya
            sbc         dhi,x
            bcc         @done
            lda         dhi,x                               ; (The next: kept over LOAD)
            pha
            lda         dlo,x
            pha
            jsr         load
            pla
            clc
            adc         #1
            dex
            sta         dlo,x
            pla
            adc         #0
            sta         dhi,x
            bra         @blk
@done:
            inx
            pla
            pla
            rts

            HEADER      "list", 0
list:                                                       ; ( u -- ): its 16 lines, numbered (SCR u)
            lda         dlo,x
            sta         bk_scr
            lda         dhi,x
            sta         bk_scr + 1
            jsr         block
            lda         #0
@line:
            pha
            jsr         cr
            pla                                             ; (Its number: 2 characters)
            pha
            cmp         #10
            lda         #' '
            bcc         :+
            lda         #'1'
:
            jsr         emit_a
            pla
            pha
            cmp         #10
            bcc         :+
            sbc         #10
:
            ora         #'0'
            jsr         emit_a
            jsr         space
            jsr         dup                                 ; ( addr addr 64 ) type, addr + 64
            dex
            lda         #64
            sta         dlo,x
            stz         dhi,x
            jsr         type
            clc
            lda         dlo,x
            adc         #64
            sta         dlo,x
            bcc         :+
            inc         dhi,x
:
            pla
            inc
            cmp         #16
            bne         @line
            inx
            jmp         cr

            HEADER      "open-blocks", 0
openblocks:                                                 ; ( c-addr u -- ): the block file that one (Gforth's):
            jsr         flush_w                             ;   the buffers written, the banks' blocks forgotten, the
            jsr         bc_forget                           ;   one before closed
            lda         bk_fd
            cmp         #$FF
            beq         :+
            stx         xsave
            jsr         CLOSE
            ldx         xsave
            lda         #$FF
            sta         bk_fd
:
            lda         dhi,x                               ; (63 characters at most)
            bne         @long
            lda         dlo,x
            cmp         #64
            bcc         :+
@long:
            lda         #63
:
            sta         bk_name
            sta         cnt
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            ldy         #0
@char:
            cpy         cnt
            beq         @done
            lda         (w),y
            iny
            sta         bk_name,y
            bra         @char
@done:
            inx
            inx
            rts

s_blocks:   .byte       9, "blocks.fb"

; ---- Its buffers

; A buffer that's block u's (the top): C = 0, .Y it; or C = 1
bk_find:
            ldy         #1
@buf:
            lda         bk_used,y
            beq         @next
            lda         bk_nlo,y
            cmp         dlo,x
            bne         @next
            lda         bk_nhi,y
            cmp         dhi,x
            bne         @next
            clc
            rts
@next:
            dey
            bpl         @buf
            sec
            rts

; .Y = a buffer for another block, free: a free one, else the one not given last (written first if it was UPDATEd)
bk_victim:
            ldy         #0
            lda         bk_used
            beq         @free
            iny
            lda         bk_used + 1
            beq         @free
            lda         bk_cur
            eor         #1
            tay
            jsr         bk_save
@free:
            lda         #0
            sta         bk_used,y
            sta         bk_upd,y
            rts

; Buffer .Y to its block if it was UPDATEd: to the block's slot in the banks, changed there (the file later); with
; no bank to be had, to the file.  Keeps .Y
bk_save:
            lda         bk_upd,y
            beq         @done
            lda         bk_used,y
            beq         @done
            sty         bk_bufi
            lda         bk_nlo,y
            sta         tmp3
            lda         bk_nhi,y
            sta         tmp3 + 1
            jsr         bc_find
            bcc         @slot
            jsr         bc_slot
            bcs         @file
@slot:
            sty         bk_sloti
            ldy         bk_bufi
            jsr         bk_addr
            ldy         bk_sloti
            sec                                             ; (The buffer to the slot)
            jsr         bc_copy
            lda         bc_flag,y
            ora         #BC_DIRTY
            sta         bc_flag,y
            bra         @saved
@file:
            ldy         bk_bufi
            jsr         bk_seek
            jsr         bk_addr
            jsr         bk_wloop
@saved:
            ldy         bk_bufi
            lda         #0
            sta         bk_upd,y
@done:
            rts

; Buffer .Y read from block u (the top: its block from here on): from its slot in the banks, else from the file (the
; rest past its end spaces), then kept in a slot.  Keeps .Y
bk_read:
            lda         dlo,x
            sta         bk_nlo,y
            sta         tmp3
            lda         dhi,x
            sta         bk_nhi,y
            sta         tmp3 + 1
            sty         bk_bufi
            jsr         bc_find
            bcs         @file
            sty         bk_sloti
            ldy         bk_bufi
            jsr         bk_addr
            ldy         bk_sloti
            clc                                             ; (The slot to the buffer)
            jsr         bc_copy
            ldy         bk_bufi
            rts
@file:
            ldy         bk_bufi
            jsr         bk_seek
            jsr         bk_addr
            jsr         bk_rloop
            jsr         bc_slot                             ; Kept in a slot, if there's a bank
            bcs         @done
            sty         bk_sloti
            ldy         bk_bufi
            jsr         bk_addr
            ldy         bk_sloti
            sec
            jsr         bc_copy
@done:
            ldy         bk_bufi
            rts

; bk_left bytes from w written to the block file (where it is).  Keeps .X, .Y
bk_wloop:
            phy
@write:
            lda         w
            sta         r0
            lda         w + 1
            sta         r0 + 1
            lda         bk_left
            sta         r1
            lda         bk_left + 1
            sta         r1 + 1
            lda         bk_fd
            stx         xsave
            jsr         WRITE
            stx         tmp + 1
            ldx         xsave
            bcc         :+
            cmp         #E_INTR
            beq         @write
            jmp         throw_os
:
            sta         tmp
            jsr         bk_past                             ; (Past what's written)
            bne         @write
            ply
            rts

; bk_left bytes read into w from the block file (where it is), past its end spaces.  Keeps .X, .Y
bk_rloop:
            phy
@read:
            lda         w
            sta         r0
            lda         w + 1
            sta         r0 + 1
            lda         bk_left
            sta         r1
            lda         bk_left + 1
            sta         r1 + 1
            lda         bk_fd
            stx         xsave
            jsr         READ
            stx         tmp + 1
            ldx         xsave
            bcc         :+
            cmp         #E_INTR
            beq         @read
            jmp         throw_os
:
            sta         tmp
            ora         tmp + 1
            beq         @blank
            jsr         bk_past
            bne         @read
            ply
            rts
@blank:
            ldy         #0                                  ; (The file's end: spaces, bk_left of them at w)
@space:
            lda         bk_left
            ora         bk_left + 1
            beq         @done
            lda         #' '
            sta         (w),y
            iny
            bne         :+
            inc         w + 1
:
            lda         bk_left
            bne         :+
            dec         bk_left + 1
:
            dec         bk_left
            bra         @space
@done:
            ply
            rts

; w and bk_left past tmp bytes (a read's or write's count).  Z = 1: none left
bk_past:
            clc
            lda         w
            adc         tmp
            sta         w
            lda         w + 1
            adc         tmp + 1
            sta         w + 1
            sec
            lda         bk_left
            sbc         tmp
            sta         bk_left
            lda         bk_left + 1
            sbc         tmp + 1
            sta         bk_left + 1
            ora         bk_left
            rts

; The file open, at buffer .Y's block, and bk_left a block's size (bk_seekto).  Keeps .Y
bk_seek:
            lda         bk_nlo,y
            sta         tmp3
            lda         bk_nhi,y
            sta         tmp3 + 1
; The file open (blocks.fb, or open-blocks's: made if it isn't there), at block tmp3 (u * 1024), and bk_left a
; block's size.  Keeps .Y
bk_seekto:
            jsr         bk_open
            stz         r0                                  ; (r0: (u & $3F) << 10; r1: u >> 6)
            lda         tmp3
            and         #$3F
            asl
            asl
            sta         r0 + 1
            lda         tmp3 + 1
            sta         r1 + 1
            lda         tmp3
            phy
            ldy         #6
:
            lsr         r1 + 1
            ror
            dey
            bne         :-
            sta         r1
            lda         bk_fd
            stx         xsave
            ldx         #0
            jsr         SEEK
            ldx         xsave
            ply
            bcc         :+
            jmp         throw_os
:
            lda         #<BLK_SIZE
            sta         bk_left
            lda         #>BLK_SIZE
            sta         bk_left + 1
            rts

; The block file open (bk_fd).  Keeps .Y
bk_open:
            lda         bk_fd
            cmp         #$FF
            bne         @done
            phy
            lda         bk_name                             ; Its name, zero-terminated (pathbuf)
            bne         :+
            LDR         w, s_blocks
            bra         @copy
:
            LDR         w, bk_name
@copy:
            lda         (w)
            tay
            lda         #0
            sta         pathbuf,y
@char:
            dey
            bmi         @open
            iny
            lda         (w),y
            dey
            sta         pathbuf,y
            bra         @char
@open:
            LDR         r0, pathbuf
            lda         #O_RDWR
            stx         xsave
            jsr         OPEN
            ldx         xsave
            bcc         @have
            cmp         #E_NOENT                            ; (None: made)
            bne         @fail
            LDR         r0, pathbuf
            lda         #O_RDWR
            stx         xsave
            ldx         #0
            jsr         CREATE
            ldx         xsave
            bcc         @have
@fail:
            jmp         throw_os
@have:
            sta         bk_fd
            ply
@done:
            rts

; w = buffer .Y's bytes.  Keeps .Y
bk_addr:
            lda         #<bk_buf
            sta         w
            lda         #>bk_buf
            cpy         #0
            beq         :+
            clc
            adc         #>BLK_SIZE
:
            sta         w + 1
            rts

; ---- The banks behind the buffers: a slot a block, 8 a bank (slot s: bank bc_bank + s / 8, at $8000 + s % 8 * 1K)

; Block tmp3's slot: C = 0, .Y it; or C = 1.  Keeps .X
bc_find:
            lda         bc_nbank                            ; (The banks' slots)
            asl
            asl
            asl
            tay
@slot:
            dey
            cpy         #$FF
            beq         @none
            lda         bc_flag,y
            beq         @slot
            lda         bc_nlo,y
            cmp         tmp3
            bne         @slot
            lda         bc_nhi,y
            cmp         tmp3 + 1
            bne         @slot
            clc
            rts
@none:
            sec
            rts

; A slot for block tmp3, which hasn't one: a free one; else a bank more (BANKS_ALLOC); else the next round (to the
; file first, if it's changed).  OUT: C = 0, .Y it, its block tmp3; or C = 1: no bank to be had.  Keeps .X
bc_slot:
            lda         bc_none
            bne         @none
            lda         bc_nbank
            asl
            asl
            asl
            tay
@free:
            dey
            cpy         #$FF
            beq         @more
            lda         bc_flag,y
            bne         @free
            bra         @take
@more:
            lda         bc_nbank
            cmp         #BC_BANKS
            bcs         @round
            lda         #1                                  ; A bank more
            stx         xsave
            jsr         BANKS_ALLOC
            ldx         xsave
            bcs         @nobank
            ldy         bc_nbank
            sta         bc_bank,y
            inc         bc_nbank
            tya                                             ; (Its first slot)
            asl
            asl
            asl
            tay
            bra         @take
@nobank:
            lda         bc_nbank                            ; (None at all: the file alone, from now on)
            bne         @round
            inc         bc_none
@none:
            sec
            rts
@round:
            lda         bc_nbank                            ; The next round
            asl
            asl
            asl
            sta         cnt
            ldy         bc_next
            cpy         cnt
            bcc         :+
            ldy         #0
:
            tya
            inc         a
            sta         bc_next
            jsr         bc_clean
@take:
            lda         tmp3
            sta         bc_nlo,y
            lda         tmp3 + 1
            sta         bc_nhi,y
            lda         #BC_HAS
            sta         bc_flag,y
            clc
            rts

; Slot .Y to the file if it's changed (written straight from its bank).  Keeps .X, .Y, w, tmp3
bc_clean:
            lda         bc_flag,y
            bpl         @done
            lda         w
            pha
            lda         w + 1
            pha
            lda         tmp3
            pha
            lda         tmp3 + 1
            pha
            lda         RAM_BANK
            pha
            lda         bc_nlo,y
            sta         tmp3
            lda         bc_nhi,y
            sta         tmp3 + 1
            jsr         bk_seekto
            jsr         bc_map
            lda         w2
            sta         w
            lda         w2 + 1
            sta         w + 1
            jsr         bk_wloop
            lda         bc_flag,y
            and         #$FF ^ BC_DIRTY
            sta         bc_flag,y
            pla
            sta         RAM_BANK
            pla
            sta         tmp3 + 1
            pla
            sta         tmp3
            pla
            sta         w + 1
            pla
            sta         w
@done:
            rts

; Slot .Y's bank selected ($00), w2 its 1K at the window.  Keeps .X, .Y
bc_map:
            tya
            and         #7
            asl
            asl
            ora         #>BANK_WINDOW
            sta         w2 + 1
            stz         w2
            tya
            lsr
            lsr
            lsr
            phy
            tay
            lda         bc_bank,y
            sta         RAM_BANK
            ply
            rts

; Slot .Y's 1K and the buffer at w: C = 0 from the slot, C = 1 to it (its bank selected meanwhile, the one there
; before put back).  Keeps .X, .Y, w
bc_copy:
            lda         #0
            rol
            sta         cnt                                 ; (1: to the slot)
            lda         RAM_BANK
            pha
            jsr         bc_map
            lda         w
            sta         w3
            lda         w + 1
            sta         w3 + 1
            phy
            phx
            ldx         #>BLK_SIZE
            ldy         #0
            lda         cnt
            bne         @out
@in:
            lda         (w2),y
            sta         (w3),y
            iny
            bne         @in
            inc         w2 + 1
            inc         w3 + 1
            dex
            bne         @in
            bra         @done
@out:
            lda         (w3),y
            sta         (w2),y
            iny
            bne         @out
            inc         w2 + 1
            inc         w3 + 1
            dex
            bne         @out
@done:
            plx
            ply
            pla
            sta         RAM_BANK
            rts

; Every changed slot to the file (save-buffers', flush's, the program's end)
bc_flush:
            lda         bc_nbank
            asl
            asl
            asl
            tay
@slot:
            dey
            cpy         #$FF
            beq         @done
            jsr         bc_clean
            bra         @slot
@done:
            rts

; The slots forgotten, the banks kept (another file's blocks from now on: open-blocks's)
bc_forget:
            ldy         #BC_SLOTS - 1
            lda         #0
:
            sta         bc_flag,y
            dey
            bpl         :-
            stz         bc_next
            rts

; ---- The core's calls

; src_addr = block src_blk's buffer (LOAD's, REFILL's next, a source nested in a block's ended: the core's, by
; blk_vec's 0), src_len its size.  Keeps .X
bk_src:
            dex
            lda         src_blk
            sta         dlo,x
            lda         src_blk + 1
            sta         dhi,x
            jsr         block
            lda         dlo,x
            sta         src_addr
            lda         dhi,x
            sta         src_addr + 1
            inx
            lda         #<BLK_SIZE
            sta         src_len
            lda         #>BLK_SIZE
            sta         src_len + 1
            clc
            rts

; The core's (blk_vec): .A 0, a block source's buffer (bk_src); 1, the program's end: the changed blocks written to
; the file (save-buffers).  Keeps .X
bk_vec:
            cmp         #1
            bne         bk_src
            jsr         savebuffers
            clc
            rts

; lib_init: the block file not open yet, the buffers free, no banks yet; the core asks this library for a block
; source's buffer, and to write the changed blocks as the program ends
lib_init:
            lda         #$FF
            sta         bk_fd
            stz         bk_name
            lda         #<bk_vec
            sta         blk_vec
            lda         #>bk_vec
            sta         blk_vec + 1
            rts
