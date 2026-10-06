; ****************************************************************************
; memory.s - HyForth's Memory-Allocation word set (/lib/forth/memory.fl: lib memory): allocate, free, resize.  The
; heap is the top of the dictionary's space, heap_lo (the core's) to DICT_END: it grows down as it's wanted, and gives
; its lowest pages back as they're freed; the dictionary ends below heap_lo's page (allot, a library's load, the
; shell's strings, unused).  A block: a cell, its size (the cell's 2 bytes with it, even), bit 0 set while it's in
; use, then its bytes (the address given); first fit, the free blocks after one another joined as a search passes
; them, and a block split when 4 bytes or more are left over.  free and resize take only an address allocate gave
; (else the ior, the heap as it was).  The iors: -59 ALLOCATE, -60 FREE, -61 RESIZE (Forth 2012's codes).  At the
; shell's prompt, free shadows the free program: % free runs it.

.include "forthlib.inc"

            HEADER      "allocate", 0
allocate:                                                   ; ( u -- a-addr ior )
            jsr         blk_size
            bcs         @fail
            jsr         blk_find
            bcc         :+
            jsr         heap_grow
            bcs         @fail
:
            jsr         blk_take
            clc
            lda         w
            adc         #2
            sta         dlo,x
            lda         w + 1
            adc         #0
            sta         dhi,x
            dex
            jmp         zero_tos
@fail:
            stz         dlo,x
            stz         dhi,x
            dex
            lda         #<-59
            sta         dlo,x
            lda         #$FF
            sta         dhi,x
            rts

            HEADER      "free", 0
free:                                                       ; ( a-addr -- ior )
            jsr         blk_of
            bcs         @bad
            lda         (w)
            and         #$FE
            sta         (w)
            jsr         heap_shrink
            jmp         zero_tos
@bad:
            lda         #<-60
            sta         dlo,x
            lda         #$FF
            sta         dhi,x
            rts

            HEADER      "resize", 0
resize:                                                     ; ( a-addr1 u -- a-addr2 ior ): failed, a-addr1 as it was
            jsr         blk_size                            ; (tmp: the size wanted)
            inx
            bcs         @fail
            jsr         blk_of                              ; (w: its block)
            bcs         @fail
            jsr         blk_avail                           ; Room where it is (the free blocks after it too)?
            lda         tmp2
            cmp         tmp
            lda         tmp2 + 1
            sbc         tmp + 1
            bcc         @move
            jsr         blk_take
            dex
            jmp         zero_tos
@move:
            lda         w                                   ; Else a new block, its bytes copied, the old one freed
            sta         w3
            lda         w + 1
            sta         w3 + 1
            jsr         blk_find
            bcc         :+
            jsr         heap_grow
            bcs         @fail
:
            jsr         blk_take
            lda         w3 + 1                              ; ( a-addr1 a-addr2 u ) move, the old block and a-addr2
            pha                                             ;   kept meanwhile
            lda         w3
            pha
            dex
            clc
            lda         w
            adc         #2
            sta         dlo,x
            pha
            lda         w + 1
            adc         #0
            sta         dhi,x
            pha
            dex
            sec
            lda         (w3)
            and         #$FE
            sbc         #2
            sta         dlo,x
            ldy         #1
            lda         (w3),y
            sbc         #0
            sta         dhi,x
            jsr         move
            dex
            pla
            sta         dhi,x
            pla
            sta         dlo,x
            pla
            sta         w
            pla
            sta         w + 1
            lda         (w)
            and         #$FE
            sta         (w)
            jsr         heap_shrink
            dex
            jmp         zero_tos
@fail:
            dex
            lda         #<-61
            sta         dlo,x
            lda         #$FF
            sta         dhi,x
            rts

; tmp = the block's size for u bytes (the top): u rounded up to even, and its cell, 4 at least.  C = 1: too big
; (more than the dictionary's space)
blk_size:
            lda         dhi,x
            cmp         #>DICT_END
            bcs         @done
            clc
            lda         dlo,x
            adc         #3
            and         #$FE
            sta         tmp
            lda         dhi,x
            adc         #0
            sta         tmp + 1
            bne         :+
            lda         tmp
            cmp         #4
            bcs         :+
            lda         #4
            sta         tmp
:
            clc
@done:
            rts

; w = the first free block of tmp bytes or more (the free blocks after it joined to it), tmp2 its size: C = 0; or
; C = 1, none
blk_find:
            lda         heap_lo
            sta         w
            lda         heap_lo + 1
            sta         w + 1
@blk:
            lda         w + 1
            cmp         #>DICT_END
            bcs         @none
            lda         (w)
            lsr
            bcs         @next
            jsr         blk_join
            lda         tmp2
            cmp         tmp
            lda         tmp2 + 1
            sbc         tmp + 1
            bcs         @found
@next:
            jsr         blk_next
            bcc         @blk
@none:
            sec
            rts
@found:
            clc
            rts

; w = a new block of tmp bytes at the heap's start (heap_lo lowered: a free block there taken into it), tmp2 its
; size: C = 0; or C = 1, no room (the heap's page at least 2 past HERE's)
heap_grow:
            stz         tmp3                                ; (tmp3: the free block at the start's size, or 0)
            stz         tmp3 + 1
            lda         heap_lo + 1
            cmp         #>DICT_END
            bcs         :+
            lda         heap_lo
            sta         w
            lda         heap_lo + 1
            sta         w + 1
            lda         (w)
            lsr
            bcs         :+
            lda         (w)
            sta         tmp3
            ldy         #1
            lda         (w),y
            sta         tmp3 + 1
:
            sec                                             ; w = heap_lo less what's wanted past that block
            lda         tmp
            sbc         tmp3
            sta         w
            lda         tmp + 1
            sbc         tmp3 + 1
            sta         w + 1
            sec
            lda         heap_lo
            sbc         w
            sta         w
            lda         heap_lo + 1
            sbc         w + 1
            sta         w + 1
            bcc         @no
            sec                                             ; (Room?)
            sbc         here + 1
            bcc         @no
            cmp         #2
            bcc         @no
            lda         w
            sta         heap_lo
            lda         w + 1
            sta         heap_lo + 1
            lda         tmp
            sta         tmp2
            lda         tmp + 1
            sta         tmp2 + 1
            clc
            rts
@no:
            sec
            rts

; Block w (tmp2 bytes there) made a block of tmp bytes in use, what's left over after it a free block if it's 4 bytes
; or more (else the block takes it)
blk_take:
            sec
            lda         tmp2
            sbc         tmp
            sta         tmp3
            lda         tmp2 + 1
            sbc         tmp + 1
            sta         tmp3 + 1
            bne         @split
            lda         tmp3
            cmp         #4
            bcs         @split
            lda         tmp2
            ora         #1
            sta         (w)
            ldy         #1
            lda         tmp2 + 1
            sta         (w),y
            rts
@split:
            clc
            lda         w
            adc         tmp
            sta         w2
            lda         w + 1
            adc         tmp + 1
            sta         w2 + 1
            lda         tmp3
            sta         (w2)
            ldy         #1
            lda         tmp3 + 1
            sta         (w2),y
            lda         tmp
            ora         #1
            sta         (w)
            lda         tmp + 1
            sta         (w),y
            rts

; w = the block in use whose bytes start at the address on top (kept): C = 0; or C = 1, not one (the heap's blocks
; read from its start)
blk_of:
            sec
            lda         dlo,x
            sbc         #2
            sta         w2
            lda         dhi,x
            sbc         #0
            sta         w2 + 1
            lda         heap_lo
            sta         w
            lda         heap_lo + 1
            sta         w + 1
@blk:
            lda         w + 1
            cmp         #>DICT_END
            bcs         @no
            lda         w
            cmp         w2
            bne         @next
            lda         w + 1
            cmp         w2 + 1
            bne         @next
            lda         (w)
            lsr
            bcc         @no
            clc
            rts
@next:
            jsr         blk_next
            bcc         @blk
@no:
            sec
            rts

; w past its block (tmp2: its size).  C = 1: a size of 0 (not a block: nothing past it)
blk_next:
            lda         (w)
            and         #$FE
            sta         tmp2
            ldy         #1
            lda         (w),y
            sta         tmp2 + 1
            ora         tmp2
            beq         @zero
            clc
            lda         w
            adc         tmp2
            sta         w
            lda         w + 1
            adc         tmp2 + 1
            sta         w + 1
            clc
            rts
@zero:
            sec
            rts

; tmp2 = block w's size and the sizes of the free blocks right after it
blk_avail:
            lda         (w)
            and         #$FE
            sta         tmp2
            ldy         #1
            lda         (w),y
            sta         tmp2 + 1
@next:
            clc
            lda         w
            adc         tmp2
            sta         w2
            lda         w + 1
            adc         tmp2 + 1
            sta         w2 + 1
            cmp         #>DICT_END
            bcs         @done
            lda         (w2)
            lsr
            bcs         @done
            lda         (w2)
            ora         (w2),y
            beq         @done
            clc
            lda         (w2)
            adc         tmp2
            sta         tmp2
            lda         (w2),y
            adc         tmp2 + 1
            sta         tmp2 + 1
            bra         @next
@done:
            rts

; Free block w joined with the free blocks right after it (tmp2: its size)
blk_join:
            jsr         blk_avail
            lda         tmp2
            sta         (w)
            ldy         #1
            lda         tmp2 + 1
            sta         (w),y
            rts

; The heap's free blocks at its start given back to the dictionary (heap_lo past them)
heap_shrink:
            lda         heap_lo + 1
            cmp         #>DICT_END
            bcs         @done
            lda         heap_lo
            sta         w
            lda         heap_lo + 1
            sta         w + 1
            lda         (w)
            lsr
            bcs         @done
            jsr         blk_join
            clc
            lda         w
            adc         tmp2
            sta         heap_lo
            lda         w + 1
            adc         tmp2 + 1
            sta         heap_lo + 1
@done:
            rts
