; ****************************************************************************
; stack.inc - hylang's evaluation stack (eval.inc's), in the task's RAM, where every bank reaches it: STACK_SIZE
; bytes from a page's start, growing up, its words values or fixnums.  When it's full, its oldest SPILL bytes go to
; a bank (taken as they're wanted, up to SPILLS blocks) and the rest move down; a block comes back when what's left
; is less than a page.  So what's popped a word at a time (pop) is always in RAM, and what's dropped (drop) must be.
;   With it, the collector's roots that are hylang's own (hl_roots, gc_hook's): the machine's registers, the stack,
; the reader's levels.

STACK_SIZE      = 6144          ; The stack in the task's RAM
SPILL           = 2048          ; A block spilled to a bank
SPILLS          = 16            ; Blocks spilled at most (4 banks)

.zeropage
sp:         .res        2                                   ; Where the next word goes

.bss
stk_raw:    .res        STACK_SIZE + 255                    ; The stack, from its first page's start
stk_basep:  .res        1                                   ; (That page ...
stk_endp:   .res        1                                   ;   and the page after its end)
spilled:    .res        1                                   ; Blocks spilled ...
spill_bank: .res        SPILLS / 4                          ;   and the banks they're in (0: not taken yet)
stk_t:      .res        2                                   ; (Scratch)
nomem_err:  .res        2                                   ; The error "out of memory", made as hylang starts

.segment "DATA4"
; The stack made, empty
stk_init:
            lda         #>(stk_raw + 255)
            sta         stk_basep
            sta         sp + 1
            stz         sp
            clc
            adc         #>STACK_SIZE
            sta         stk_endp
            stz         spilled
            ldx         #SPILLS / 4 - 1
:
            stz         spill_bank,x
            dex
            bpl         :-
            rts

; .A, .X (low, high) pushed.  Keeps .A, .X, .Y
push:
            sta         (sp)
            pha
            phy
            ldy         #1
            txa
            sta         (sp),y
            ply
            pla
            inc         sp
            inc         sp
            beq         @page
            rts
@page:
            inc         sp + 1
            pha
            lda         sp + 1
            cmp         stk_endp
            pla
            bcc         @lb16
            jmp         spill
@lb16:
            rts

; OUT: .A, .X = the word popped (low, high).  Keeps .Y
pop:
            lda         sp
            bne         :+
            dec         sp + 1
:
            dec         sp
            dec         sp
            phy
            ldy         #1
            lda         (sp),y
            tax
            lda         (sp)
            ply
            pha
            lda         sp + 1
            cmp         stk_basep
            bne         @done
            lda         spilled
            beq         @done
            jsr         unspill
@done:
            pla
            rts

; The top .A words dropped (they're in RAM).  Keeps .X, .Y
drop:
            asl
            sta         stk_t
            lda         sp
            sec
            sbc         stk_t
            sta         sp
            bcs         :+
            dec         sp + 1
:
            lda         sp + 1
            cmp         stk_basep
            bne         @done
            lda         spilled
            beq         @done
            jmp         unspill
@done:
            rts

; The top cn words (a call's arguments: in RAM) dropped.  Keeps .X, .Y
drop_args:
            lda         cn
            asl
            sta         stk_t
            lda         cn + 1
            rol
            sta         stk_t + 1
            lda         sp
            sec
            sbc         stk_t
            sta         sp
            lda         sp + 1
            sbc         stk_t + 1
            sta         sp + 1
            cmp         stk_basep
            bne         @done
            lda         spilled
            beq         @done
            jmp         unspill
@done:
            rts

; OUT: argp = the address of the top cn words, a call's arguments, brought back to RAM if they were spilled; C = 1
; if they can't all be in RAM at once
args_at:
            lda         cn                                  ; (2 * cn bytes ...
            asl
            sta         stk_t
            lda         cn + 1
            rol
            sta         stk_t + 1
@again:
            lda         sp                                  ;   from sp back: in RAM if that's at or past its start)
            sec
            sbc         stk_t
            sta         argp
            lda         sp + 1
            sbc         stk_t + 1
            sta         argp + 1
            bcc         @spilled
            cmp         stk_basep
            bcs         @done
@spilled:
            lda         spilled                             ; (A block back, if there's room for it)
            beq         @no
            lda         sp + 1
            clc
            adc         #>SPILL
            bcs         @no
            cmp         stk_endp
            bcs         @no
            jsr         unspill
            bra         @again
@done:
            clc
            rts
@no:
            sec
            rts

; The stack's RAM is full: its oldest SPILL bytes to a bank, the rest moved down.  Keeps .A, .X, .Y and the window.
; No room for them: hylang's error "Too deep" (eval.inc: ev_toodeep, in its first bank)
spill:
            pha
            phx
            phy
            lda         spilled
            cmp         #SPILLS
            bcs         @deep
            lsr
            lsr
            tax
            lda         spill_bank,x
            bne         @bank
            phx
            lda         #1
            jsr         BANKS_ALLOC
            plx
            bcs         @deep
            sta         spill_bank,x
@bank:
            lda         spilled                             ; (Its place in the bank: a quarter)
            and         #3
            asl
            asl
            asl
            ora         #$80
            sta         @to + 2
            lda         stk_basep
            sta         @from + 2
            lda         $00
            pha
            lda         spill_bank,x
            sta         $00
            ldx         #>SPILL
            ldy         #0
@from:
            lda         $FF00,y
@to:
            sta         $FF00,y
            iny
            bne         @from
            inc         @from + 2
            inc         @to + 2
            dex
            bne         @from
            pla
            sta         $00
            lda         stk_basep                           ; (The rest down: from SPILL bytes on, to the end)
            sta         @dst + 2
            clc
            adc         #>SPILL
            sta         @src + 2
            ldx         #>(STACK_SIZE - SPILL)
            ldy         #0
@src:
            lda         $FF00,y
@dst:
            sta         $FF00,y
            iny
            bne         @src
            inc         @src + 2
            inc         @dst + 2
            dex
            bne         @src
            lda         sp + 1
            sec
            sbc         #>SPILL
            sta         sp + 1
            lda         argp + 1                            ; (A built-in's arguments moved down with the rest: it
            sec                                             ;   may push, as eq's do, and read them after)
            sbc         #>SPILL
            sta         argp + 1
            inc         spilled
            ply
            plx
            pla
            rts
@deep:
            lda         hyx2_bank1                          ; (The machine's first bank, and its error)
            sta         $01
            jmp         ev_toodeep

; The last block spilled brought back (spilled > 0, and what's on the stack less than SPILL from its end): what's
; there moved up, the block below it.  Keeps .A, .X, .Y and the window
unspill:
            pha
            phx
            phy
            lda         sp + 1                              ; (Up, from the top page down: pages stk_basep to sp's)
            sta         @src + 2
            clc
            adc         #>SPILL
            sta         @dst + 2
            lda         sp + 1
            sec
            sbc         stk_basep
            tax                                             ; (Pages to move: that, and one)
            inx
            ldy         #0
@src:
            lda         $FF00,y
@dst:
            sta         $FF00,y
            iny
            bne         @src
            dec         @src + 2
            dec         @dst + 2
            dex
            bne         @src
            dec         spilled                             ; (The block back)
            lda         spilled
            pha
            lsr
            lsr
            tax
            pla
            and         #3
            asl
            asl
            asl
            ora         #$80
            sta         @from + 2
            lda         stk_basep
            sta         @to + 2
            lda         $00
            pha
            lda         spill_bank,x
            sta         $00
            ldx         #>SPILL
            ldy         #0
@from:
            lda         $FF00,y
@to:
            sta         $FF00,y
            iny
            bne         @from
            inc         @from + 2
            inc         @to + 2
            dex
            bne         @from
            pla
            sta         $00
            lda         sp + 1
            clc
            adc         #>SPILL
            sta         sp + 1
            ply
            plx
            pla
            rts

; IN: .A = a kind (a cell of two values); et, eu = the values.  OUT: hv = the cell, hp its address; C = 1 and .A =
; E_NOMEM if there's no room.  It may collect
make2:
            jsr         cell_alloc
            bcs         @rts
            lda         et
            sta         (hp)
            ldy         #1
            lda         et + 1
            sta         (hp),y
            iny
            lda         eu
            sta         (hp),y
            iny
            lda         eu + 1
            sta         (hp),y
            clc
@rts:
            rts

; hylang's roots (gc_hook's): the machine's registers (ex to eu), the error kept, the stack (in RAM and spilled),
; and the reader's levels.  (The collector keeps hp to ht as they were: hq walks the stack)
hl_roots:
            ldx         #0
@reg:
            lda         ex,x
            sta         mv
            lda         ex + 1,x
            sta         mv + 1
            phx
            jsr         mark_val
            jsr         gc_drain
            plx
            inx
            inx
            cpx         #EV_ROOTS
            bcc         @reg
            lda         nomem_err
            sta         mv
            lda         nomem_err + 1
            sta         mv + 1
            jsr         mark_val
            jsr         gc_drain
            stz         hq                                  ; (The stack in RAM: its first page to sp)
            lda         stk_basep
            sta         hq + 1
@ram:
            lda         hq
            cmp         sp
            lda         hq + 1
            sbc         sp + 1
            bcs         @spilled
            lda         (hq)
            sta         mv
            ldy         #1
            lda         (hq),y
            sta         mv + 1
            jsr         mark_val
            jsr         gc_drain
            inc         hq
            inc         hq
            bne         @ram
            inc         hq + 1
            bra         @ram
@spilled:
            stz         hk                                  ; (Each block spilled: its bank, its words)
@block:
            lda         hk
            cmp         spilled
            bcs         @reader
            lsr
            lsr
            tax
            lda         spill_bank,x
            sta         hb
            lda         hk
            and         #3
            asl
            asl
            asl
            ora         #$80
            sta         hq + 1
            stz         hq
            lda         #<(SPILL / 2)
            sta         hn
            lda         #>(SPILL / 2)
            sta         hn + 1
@word:
            lda         hb
            sta         $00
            lda         (hq)
            sta         mv
            ldy         #1
            lda         (hq),y
            sta         mv + 1
            jsr         mark_val
            jsr         gc_drain
            inc         hq
            inc         hq
            bne         :+
            inc         hq + 1
:
            lda         hn
            bne         :+
            dec         hn + 1
:
            dec         hn
            lda         hn
            ora         hn + 1
            bne         @word
            inc         hk
            bra         @block
@reader:
            jmp         rd_roots
