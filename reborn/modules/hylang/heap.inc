; ****************************************************************************
; heap.inc - hylang's heap and its collector (docs/hylang.md, "The design"; phase 1), in the task's RAM (the module's
; DATA and BSS), where each of its banks calls it.  With hylang.inc: the values' and the kinds' names.
;   A value is a word.  Bit 0 set: a fixnum, n * 2 + 1 (n from -16384 to 16383).  Clear: a reference r.  Below
; IMM_PAGES * 256, r is an immediate, with no memory behind it (NIL $0000, T, (), exit, the characters, the
; built-ins).  Else r is a cell, at byte r * 2 of the cell heap (128K: 16 of the task's RAM banks, taken as they're
; needed): bits 15-12 its bank, bits 11-0 times 2 its place in the $8000-$9FFF window.  So a reference's high byte
; is its page (512 bytes): pk says what the page holds, bank_of its bank, ptr_hi its address; deref makes a
; reference its cell's address.
;   A page holds one kind of cell (PK_*), all of one size (kd_size: 4 to 64 bytes), each word a value but those
; kd_raw says aren't (a length, a blob, flags).  Each kind's free cells are a list (fl_lo/fl_hi: a free cell's first
; word the next), filled from a free page as it needs one; cell_alloc takes the first, its words NILs.
;   Blobs (a string's bytes, a bignum's, a symbol's name) are in banks of their own (up to 16), each filled upward
; from byte 4: the owner (the cell whose blob it is: kd_blob says which word), the size, then the bytes, a whole
; number of words.  A blob reference is its bank (bits 15-12) and its place in words (bits 11-0).
;   The collector (gc_collect) marks (a bit for each 4 bytes of the cell heap, in marks) from the roots: the value
; registers (VR0-VR7 and hv), the root stack (root_push, root_pop), the symbols (sym_lo/hi) and what gc_hook marks
; (the evaluator's, from phase 3).  Each cell marked is pushed on the mark stack and scanned: its values marked, its
; spine (kd_spine: a cons's cdr, a frame's parent ...) followed in place, so a list costs no stack.  A full stack is
; noted (movf), and every marked cell scanned again till nothing new is marked.  The sweep makes a page with nothing
; marked free and lists the others' unmarked cells; then the blobs whose owners aren't marked are dropped, and the
; rest slid down in their banks, each owner's reference with it.
;   Anything that allocates may collect, so a value kept across it is in a root.  The collector keeps the heap's
; scratch registers (hp to ht) as they were.

ROOTS           = 64            ; The root stack's size

.zeropage
vr:         .res        16                                  ; VR0-VR7: values kept (roots)
hv:         .res        2                                   ; A value made or found (a root too)
hp:         .res        2                                   ; An address in the window (deref's, a blob's)
hq:         .res        2                                   ; An address in the task's RAM (a name, a string's bytes)
hb:         .res        2                                   ; A blob's reference
hn:         .res        2                                   ; A size, a count
hk:         .res        1                                   ; A kind
ht:         .res        2                                   ; (Scratch)
mv:         .res        2                                   ; The collector's: a value being marked ...
mp:         .res        2                                   ;   its mark's byte, and its bit (the sweep's) ...
mb:         .res        1
mc:         .res        2                                   ;   the cell being scanned (a blob moved from) ...
ms:         .res        2                                   ;   its spine's value (a blob moved to)
mk:         .res        1                                   ;   its kind ...
mi:         .res        1                                   ;   the word at, and its words
mw:         .res        1

VR0             = vr
VR1             = vr + 2
VR2             = vr + 4
VR3             = vr + 6
VR4             = vr + 8
VR5             = vr + 10
VR6             = vr + 12
VR7             = vr + 14

PSTATE_SEG
pk:         .res        256                                 ; Each page's kind (PK_*)
fl_lo:      .res        PK_KINDS                            ; Each kind's free cells, a list
fl_hi:      .res        PK_KINDS
sym_lo:     .res        256                                 ; The symbols and atoms: buckets, each a chain (word 2)
sym_hi:     .res        256
cell_bank:  .res        16                                  ; The cell heap's banks, and how many
cell_banks: .res        1
blob_bank:  .res        16                                  ; The blobs' banks, and how many ...
blob_tlo:   .res        16                                  ;   each filled to this byte
blob_thi:   .res        16
blob_banks: .res        1
free_pages: .res        1                                   ; The cell heap's free pages
fp_next:    .res        1                                   ; (Where find_page looks first)
gc_count:   .res        2                                   ; The collections so far
gc_fresh:   .res        1                                   ; Pages made a kind's since the last (the REPL's: collect
                                                            ;   while the prompt waits)

.bss
bank_of:    .res        256                                 ; A page's bank (for $00: cell_bank's, so not PSTATE's) ...
ptr_hi:     .res        256                                 ;   and its address's high byte in the window
mlo:        .res        256                                 ; A reference's mark: its byte, its low byte's part ...
mbase_lo:   .res        256                                 ;   its page's first mark byte, from its high byte ...
mbase_hi:   .res        256
mbit:       .res        256                                 ;   and its bit, from its low byte
marks_raw:  .res        4096 + 15                           ; The marks: a bit for each 4 bytes of the cell heap, from
marks:      .res        2                                   ;   here (marks_raw's first address that's a multiple of 16)
ms_lo:      .res        256                                 ; The mark stack (255 values)
ms_hi:      .res        256
msp:        .res        1
movf:       .res        1                                   ; (The mark stack was full: scan again)
rs_lo:      .res        ROOTS                               ; The root stack
rs_hi:      .res        ROOTS
rsp:        .res        1
fp_size:    .res        1                                   ; (A page's cells' size, and their references' step:
fp_step:    .res        1                                   ;   find_page's)
gc_stress:  .res        1                                   ; <> 0: collect before every allocation (a test's: hylang -g)
gc_save:    .res        11                                  ; (The scratch registers, hp to ht, while it collects)
g_bank:     .res        1                                   ; (The blob bank being compacted: its index, its bank ...
g_index:    .res        1
g_top:      .res        2                                   ;   its top, as an address ...
g_size:     .res        2                                   ;   a blob's size in words, as bytes ...
g_word:     .res        1                                   ;   and its owner's word)
b_tried:    .res        1                                   ; (blob_alloc's: it collected)
cg_y:       .res        1                                   ; (cell_get's: .Y kept)
i_kind:     .res        1                                   ; (intern's: the kind, the name's length, the bucket,
i_len:      .res        1                                   ;   the next in it)
i_hash:     .res        1
i_next:     .res        2

.ifdef HL_DATA4
.segment "DATA4"                                            ; (hylang's: from its fourth bank, hylang.cfg)
.else
.segment "DATA"                                             ; (A module of one bank's: t_heap)
.endif

; ---- Making the heap

; The heap made: its tables, two banks of cells and one of blobs.  OUT: C = 1, .A the error, if there's no room
heap_init:
            jsr         heap_tables
            ldx         #0
@pages:
            stz         pk,x                                ; (PK_NONE)
            stz         bank_of,x
            stz         sym_lo,x
            stz         sym_hi,x
            inx
            bne         @pages
            ldx         #PK_KINDS - 1
@lists:
            stz         fl_lo,x
            stz         fl_hi,x
            dex
            bpl         @lists
            stz         cell_banks
            stz         blob_banks
            stz         free_pages
            stz         fp_next
            stz         gc_count
            stz         gc_count + 1
            stz         gc_fresh
            jsr         take_cell_bank
            bcs         @rts
            jsr         take_cell_bank
            bcs         @rts
            jmp         take_blob_bank
@rts:
            rts

; The heap's tables that its layout gives (the marks' places, the pages' addresses), and what's kept while it works
; (the value registers NIL, the root stack and the mark stack empty): heap_init's, and a snapshot's (snap_restore)
heap_tables:
            lda         #<(marks_raw + 15)                  ; (The marks from a multiple of 16)
            and         #$F0
            sta         marks
            lda         #>(marks_raw + 15)
            sta         marks + 1
            ldx         #0                                  ; (The first 16 ...
@first:
            txa
            asl
            asl
            asl
            asl
            clc
            adc         marks
            sta         mbase_lo,x                          ; (A page's first mark byte: marks + 16 * x ...
            lda         marks + 1
            adc         #0
            sta         mbase_hi,x
            stz         mlo,x                               ;   and x >> 4, a low byte's, ORed in)
            txa
            lsr
            tay
            lda         bit_of,y
            sta         mbit,x                              ; (1 << ((x >> 1) & 7): the bit)
            txa
            asl
            ora         #$80
            sta         ptr_hi,x                            ; (((x & $0F) << 1) | $80: a page's address)
            inx
            cpx         #16
            bne         @first
            ldx         #0                                  ;   then each from the one 16 before: the same, but the
@rest:                                                      ;   mark byte's high byte and mlo one more)
            lda         ptr_hi,x
            sta         ptr_hi + 16,x
            lda         mbit,x
            sta         mbit + 16,x
            lda         mbase_lo,x
            sta         mbase_lo + 16,x
            lda         mbase_hi,x
            inc         a
            sta         mbase_hi + 16,x
            lda         mlo,x
            inc         a
            sta         mlo + 16,x
            inx
            cpx         #256 - 16
            bne         @rest
            ldx         #17                                 ; (VR0-VR7 and hv: NIL)
@values:
            stz         vr,x
            dex
            bpl         @values
            stz         rsp
            stz         msp
            stz         movf
            stz         gc_stress
            rts

; Another bank for the cells: its 16 pages free (the first bank's first IMM_PAGES the immediates').  OUT: C = 1,
; .A the error (E_NOMEM: 16 already, or no bank left)
take_cell_bank:
            lda         cell_banks
            cmp         #16
            bcs         @full
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @rts
            ldx         cell_banks
            sta         cell_bank,x
            sta         ht
            txa
            asl
            asl
            asl
            asl
            tax                                             ; (Its first page)
            ldy         #16
@page:
            lda         ht
            sta         bank_of,x
            lda         #PK_IMM
            cpx         #IMM_PAGES
            bcc         :+
            lda         #PK_FREE
            inc         free_pages
:
            sta         pk,x
            inx
            dey
            bne         @page
            inc         cell_banks
            clc
@rts:
            rts
@full:
            lda         #E_NOMEM
            sec
            rts

; Another bank for the blobs, empty (filled from byte 4, so no blob's reference is 0).  OUT: C = 1, .A the error
take_blob_bank:
            lda         blob_banks
            cmp         #16
            bcs         @full
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @rts
            ldx         blob_banks
            sta         blob_bank,x
            lda         #4
            sta         blob_tlo,x
            stz         blob_thi,x
            inc         blob_banks
            clc
@rts:
            rts
@full:
            lda         #E_NOMEM
            sec
            rts

; ---- Values

; IN: .A, .X = a reference (low, high).  OUT: hp = its cell's address, its bank in the window ($00).  Keeps .X, .Y
deref:
            sta         hp
            lda         bank_of,x
            sta         $00
            lda         hp
            asl
            sta         hp
            lda         ptr_hi,x
            adc         #0                                  ; (The place's bit 8, from the low byte's bit 7)
            sta         hp + 1
            rts

; IN: .A, .X = a value.  OUT: .A = its kind: PK_FIXNUM, PK_IMM (an immediate), or its cell's.  Keeps .X, .Y
kind_of:
            lsr
            bcs         @fixnum
            lda         pk,x
            rts
@fixnum:
            lda         #PK_FIXNUM
            rts

; IN: .A, .X = a number (low, high).  OUT: hv = it, a fixnum; C = 1 if it isn't in a fixnum's range.  Keeps .Y
fix_make:
            sta         hv
            stx         hv + 1
            txa
            asl                                             ; (Bits 15 and 14 the same: in range)
            eor         hv + 1
            bmi         @out
            asl         hv
            rol         hv + 1
            lda         hv
            ora         #1
            sta         hv
            clc
            rts
@out:
            sec
            rts

; IN: hv = a fixnum.  OUT: .A, .X = its number (low, high).  Keeps .Y
fix_value:
            lda         hv + 1
            cmp         #$80                                ; (The sign, into C)
            ror
            tax
            lda         hv
            ror
            rts

; ---- Cells

; IN: .A = a kind (a cell's).  OUT: hv = a new cell of it, its words NIL (hp its address, in the window); C = 1 and
; .A = E_NOMEM if there's no room.  It may collect (and with gc_stress, does)
cell_alloc:
            sta         hk
            lda         gc_stress
            beq         @again
            GC_CALL
@again:
            ldx         hk
            lda         fl_lo,x
            ora         fl_hi,x
            beq         @empty
            lda         fl_lo,x                             ; (The first free cell ...
            sta         hv
            lda         fl_hi,x
            sta         hv + 1
            phx
            tax
            lda         hv
            jsr         deref
            plx
            lda         (hp)                                ;   its first word the next)
            sta         fl_lo,x
            ldy         #1
            lda         (hp),y
            sta         fl_hi,x
            lda         kd_size,x
            tay
            dey
            lda         #0
@clear:
            sta         (hp),y
            dey
            bpl         @clear
            clc
            rts
@empty:
            jsr         find_page                           ; (A free page for the kind ...
            bcc         @again
            GC_CALL                          ;   none: a collection ...
            jsr         maybe_grow
            ldx         hk
            lda         fl_lo,x
            ora         fl_hi,x
            bne         @again
            jsr         find_page
            bcc         @again
            jsr         take_cell_bank                      ;   still none: a bank more)
            bcs         @nomem
            jsr         find_page
            bcc         @again
@nomem:
            lda         #E_NOMEM
            sec
            rts

; After a collection, a bank more if fewer of the cell heap's pages are free than are used: a collection's work is
; what's live, so with as much free again it costs each cell made about the same, however much is live (the
; library's, as hylang starts, half its first two banks).  Its failure doesn't matter
maybe_grow:
            lda         cell_banks
            cmp         #16
            bcs         @rts
            asl
            asl
            asl
            asl
            sec
            sbc         #IMM_PAGES
            sec
            sbc         free_pages                          ; (The pages used)
            cmp         free_pages
            bcc         @rts
            jsr         take_cell_bank
@rts:
            rts

; A free page made hk's: its cells its free list.  OUT: C = 1 if there's no free page
find_page:
            ldx         fp_next
            ldy         #0
@look:
            lda         pk,x
            cmp         #PK_FREE
            beq         @found
            inx
            dey
            bne         @look
            sec
            rts
@found:
            stx         fp_next
            lda         hk
            sta         pk,x
            dec         free_pages
            inc         gc_fresh
            bne         :+
            dec         gc_fresh                            ; (255 at most)
:
            stx         ht + 1                              ; (Its first cell)
            stz         ht
            lda         #0
            jsr         deref
            ldx         hk
            lda         kd_size,x
            sta         fp_size                             ; (A cell's size ...
            lsr
            sta         fp_step                             ;   and its references' step)
@cell:
            lda         fl_lo,x
            sta         (hp)
            lda         fl_hi,x
            ldy         #1
            sta         (hp),y
            lda         ht
            sta         fl_lo,x
            lda         ht + 1
            sta         fl_hi,x
            lda         hp
            clc
            adc         fp_size
            sta         hp
            bcc         :+
            inc         hp + 1
:
            lda         ht
            clc
            adc         fp_step
            sta         ht
            bne         @cell                               ; (To the page's end, where it wraps)
            clc
            rts

; IN: .A, .X = a cell's reference; .Y = a word (0-31).  OUT: .A, .X = the word (low, high).  Keeps .Y
cell_get:
            sta         hp                                  ; (deref's, here)
            lda         bank_of,x
            sta         $00
            lda         hp
            asl
            sta         hp
            lda         ptr_hi,x
            adc         #0
            sta         hp + 1
            sty         cg_y
            tya
            asl
            tay
            iny
            lda         (hp),y
            tax
            dey
            lda         (hp),y
            ldy         cg_y
            rts

; IN: hv = a cell's reference; .Y = a word; .A, .X = the value for it.  Keeps .Y
cell_set:
            sta         ht
            stx         ht + 1
            lda         hv
            ldx         hv + 1
            jsr         deref
            phy
            tya
            asl
            tay
            lda         ht
            sta         (hp),y
            iny
            lda         ht + 1
            sta         (hp),y
            ply
            rts

; ---- The roots

; IN: .A, .X = a value, kept (a root) till root_pop.  OUT: C = 1 if the root stack is full.  Keeps .Y
root_push:
            phy
            ldy         rsp
            cpy         #ROOTS
            bcs         @full
            sta         rs_lo,y
            txa
            sta         rs_hi,y
            inc         rsp
            ply
            clc
            rts
@full:
            ply
            sec
            rts

; OUT: .A, .X = the value root_push kept last, no longer kept.  Keeps .Y
root_pop:
            phy
            dec         rsp
            ldy         rsp
            lda         rs_hi,y
            tax
            lda         rs_lo,y
            ply
            rts

; ---- Blobs

; IN: hn = a size (bytes, up to $1FF8); hv = its owner (a cell).  OUT: hb = a new blob's reference, its owner and
; size written; C = 1 and .A = E_NOMEM if there's no room.  It may collect
blob_alloc:
            lda         hn                                  ; (The whole: the header and the bytes, in words)
            clc
            adc         #5
            and         #$FE
            sta         ht
            lda         hn + 1
            adc         #0
            sta         ht + 1
            cmp         #$20
            bcs         @nomem                              ; (Bigger than a bank)
            stz         b_tried
            lda         gc_stress
            beq         @try
            GC_CALL
@try:
            ldx         #0
@bank:
            cpx         blob_banks
            bcs         @none
            lda         blob_tlo,x                          ; (Its top, after the blob: $2000 at most)
            clc
            adc         ht
            sta         hb
            lda         blob_thi,x
            adc         ht + 1
            cmp         #$20
            bcc         @fits
            bne         @next
            lda         hb
            beq         @fits
@next:
            inx
            bra         @bank
@none:
            lda         b_tried                             ; (No room: a collection, then a bank more)
            bne         @grow
            inc         b_tried
            GC_CALL
            bra         @try
@grow:
            jsr         take_blob_bank
            bcc         @try
@nomem:
            lda         #E_NOMEM
            sec
            rts
@fits:
            lda         blob_bank,x                         ; (Its header: at the bank's top)
            sta         $00
            lda         blob_tlo,x
            sta         hp
            sta         hb
            lda         blob_thi,x
            ora         #$80
            sta         hp + 1
            lda         blob_thi,x                          ; (Its reference: the bank, the place in words)
            lsr
            ror         hb
            sta         hb + 1
            txa
            asl
            asl
            asl
            asl
            ora         hb + 1
            sta         hb + 1
            lda         hv                                  ; (The owner, the size)
            sta         (hp)
            ldy         #1
            lda         hv + 1
            sta         (hp),y
            iny
            lda         hn
            sta         (hp),y
            iny
            lda         hn + 1
            sta         (hp),y
            lda         blob_tlo,x                          ; (The bank's top past it)
            clc
            adc         ht
            sta         blob_tlo,x
            lda         blob_thi,x
            adc         ht + 1
            sta         blob_thi,x
            clc
            rts

; IN: .A, .X = a blob's reference.  OUT: hp = its header's address, its bank in the window.  Keeps .Y
blob_deref:
            sta         hp
            txa
            lsr
            lsr
            lsr
            lsr
            phx
            tax
            lda         blob_bank,x
            sta         $00
            plx
            txa
            and         #$0F
            sta         hp + 1
            asl         hp
            rol         hp + 1
            lda         hp + 1
            ora         #$80
            sta         hp + 1
            rts

; IN: .A, .X = a blob's reference.  OUT: hp = its bytes' address, its bank in the window; hn = its size.  Keeps .Y
blob_data:
            jsr         blob_deref
            phy
            ldy         #2
            lda         (hp),y
            sta         hn
            iny
            lda         (hp),y
            sta         hn + 1
            ply
            lda         hp
            clc
            adc         #4
            sta         hp
            bcc         :+
            inc         hp + 1
:
            rts

; IN: hq = bytes in the task's RAM (or the caller's bank), hn = how many.  OUT: hv = a string of them; C = 1 and
; .A = E_NOMEM if there's no room.  It may collect
string_make:
            jsr         string_alloc
            bcs         @rts
            jsr         copy_in
            clc
@rts:
            rts

; IN: hn = a length.  OUT: hv = a string of that many bytes, not set yet: hp their address, in the window; C = 1 and
; .A = E_NOMEM if there's no room.  It may collect.  (kind_alloc: .A = PK_STRING or PK_BUFFER, a buffer's the same)
string_alloc:
            lda         #PK_STRING
kind_alloc:
            jsr         cell_alloc
            bcs         @rts
            jsr         blob_alloc
            bcs         @rts
            lda         hv                                  ; (Its length, its blob)
            ldx         hv + 1
            jsr         deref
            lda         hn
            sta         (hp)
            ldy         #1
            lda         hn + 1
            sta         (hp),y
            iny
            lda         hb
            sta         (hp),y
            iny
            lda         hb + 1
            sta         (hp),y
            lda         hb                                  ; (Its bytes)
            ldx         hb + 1
            jsr         blob_data
            clc
@rts:
            rts

; IN: hq = a message's bytes in the task's RAM (or the caller's bank), hn = how many.  OUT: hv = an error of it (its
; code NIL); C = 1 and .A = E_NOMEM if there's no room.  It may collect
error_make:
            jsr         string_make
            bcc         error_wrap
            rts
; IN: hv = a string.  OUT: hv = an error, the string its message (its code NIL); C = 1 and .A = E_NOMEM if there's
; no room.  It may collect
error_wrap:
            lda         hv                                  ; (The message kept while the error's made)
            ldx         hv + 1
            jsr         root_push
            bcs         @nomem
            lda         #PK_ERROR
            jsr         cell_alloc
            jsr         root_pop                            ; (Keeps C)
            bcs         @nomem
            sta         (hp)                                ; (Its message)
            txa
            ldy         #1
            sta         (hp),y
            clc
            rts
@nomem:
            lda         #E_NOMEM
            sec
            rts

; hn bytes from hq (the task's RAM) to hp (the window).  Keeps hq, hp, hn
copy_in:
            lda         hq + 1
            pha
            lda         hp + 1
            pha
            ldy         #0
            ldx         hn + 1
            beq         @last
@pages:
            lda         (hq),y
            sta         (hp),y
            iny
            bne         @pages
            inc         hq + 1
            inc         hp + 1
            dex
            bne         @pages
@last:
            ldx         hn
            beq         @done
@bytes:
            lda         (hq),y
            sta         (hp),y
            iny
            dex
            bne         @bytes
@done:
            pla
            sta         hp + 1
            pla
            sta         hq + 1
            rts

; ---- Symbols and atoms

; IN: .A = PK_SYMBOL or PK_ATOM; hq = a name in the task's RAM, hn = its length (1-255).  OUT: hv = the symbol (or
; atom) of that name, the one there is or a new one (its value UNBOUND); C = 1 and .A = E_NOMEM if there's no room.
; It may collect
intern:
            sta         i_kind
            lda         hn
            sta         i_len
            lda         #0                                  ; (The name's hash: each byte, the hash rotated left)
            ldy         #0
@hash:
            cpy         hn
            beq         @hashed
            cmp         #$80
            rol
            eor         (hq),y
            iny
            bra         @hash
@hashed:
            sta         i_hash
            tax
            lda         sym_lo,x
            sta         hv
            lda         sym_hi,x
            sta         hv + 1
@chain:
            lda         hv
            ora         hv + 1
            beq         @new
            lda         hv
            ldx         hv + 1
            jsr         deref
            ldy         #4                                  ; (The next in the bucket)
            lda         (hp),y
            sta         i_next
            iny
            lda         (hp),y
            sta         i_next + 1
            lda         pk,x
            cmp         i_kind
            bne         @next
            lda         (hp)                                ; (Its name: its size in hn, its bytes at hp)
            pha
            ldy         #1
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            lda         hn + 1
            bne         @next
            lda         hn
            cmp         i_len
            bne         @next
            jsr         same_name
            bcc         @found
@next:
            lda         i_next
            sta         hv
            lda         i_next + 1
            sta         hv + 1
            bra         @chain
@found:
            lda         i_len                               ; (hn as it was given)
            sta         hn
            stz         hn + 1
            clc
            rts
@new:
            lda         i_len
            sta         hn
            stz         hn + 1
            lda         i_kind
            jsr         cell_alloc
            bcs         @rts
            jsr         blob_alloc
            bcs         @rts
            lda         hv                                  ; (Its name, its value, the bucket's chain)
            ldx         hv + 1
            jsr         deref
            lda         hb
            sta         (hp)
            ldy         #1
            lda         hb + 1
            sta         (hp),y
            iny
            lda         #<UNBOUND
            sta         (hp),y
            iny
            lda         #>UNBOUND
            sta         (hp),y
            iny
            ldx         i_hash
            lda         sym_lo,x
            sta         (hp),y
            iny
            lda         sym_hi,x
            sta         (hp),y
            lda         hv
            sta         sym_lo,x
            lda         hv + 1
            sta         sym_hi,x
            lda         hb                                  ; (Its name's bytes)
            ldx         hb + 1
            jsr         blob_data
            jsr         copy_in
            clc
@rts:
            rts

; A name's bytes in a blob (hp) against the name looked for (hq), i_len of each.  OUT: C = 0 if they're the same
same_name:
            ldy         #0
@byte:
            cpy         i_len
            beq         @yes
            lda         (hp),y
            cmp         (hq),y
            bne         @no
            iny
            bra         @byte
@yes:
            clc
            rts
@no:
            sec
            rts

; ---- The collector (GC_SEG: hylang's sixth bank; its roots' hooks, in the RAM, call mark_val and gc_drain there)

GC_SEG

; A collection: the cells nothing reaches taken back, the blobs nothing owns dropped (the rest moved down).  The
; scratch registers (hp to ht) are kept
gc_collect:
            ldx         #10
@save:
            lda         hp,x
            sta         gc_save,x
            dex
            bpl         @save
            inc         gc_count
            bne         :+
            inc         gc_count + 1
:
            stz         gc_fresh
            lda         marks                               ; (The marks cleared: 256 bytes a bank taken)
            sta         mp
            lda         marks + 1
            sta         mp + 1
            ldx         cell_banks
            ldy         #0
            tya
@clear:
            sta         (mp),y
            iny
            bne         @clear
            inc         mp + 1
            dex
            bne         @clear
            stz         msp
            stz         movf
            ldx         #0                                  ; (The value registers)
@values:
            lda         vr,x
            sta         mv
            lda         vr + 1,x
            sta         mv + 1
            phx
            jsr         mark_val
            plx
            inx
            inx
            cpx         #18
            bcc         @values
            jsr         gc_drain
            ldx         #0                                  ; (The root stack)
@roots:
            cpx         rsp
            bcs         @symbols
            lda         rs_lo,x
            sta         mv
            lda         rs_hi,x
            sta         mv + 1
            phx
            jsr         mark_val
            jsr         gc_drain
            plx
            inx
            bra         @roots
@symbols:
            ldx         #0                                  ; (The symbols' buckets: those that aren't empty)
@bucket:
            lda         sym_hi,x
            beq         @nextbucket                         ; (NIL: none)
            sta         mv + 1
            lda         sym_lo,x
            sta         mv
            phx
            jsr         mark_val
            jsr         gc_drain
            plx
@nextbucket:
            inx
            bne         @bucket
            jsr         gc_roots                            ; (The evaluator's)
            jsr         gc_drain
@rescan:
            lda         movf                                ; (The mark stack was full: every marked cell again)
            beq         @marked
            stz         movf
            jsr         gc_rescan
            bra         @rescan
@marked:
            jsr         gc_sweep
            jsr         gc_blobs
            ldx         #10
@back:
            lda         gc_save,x
            sta         hp,x
            dex
            bpl         @back
            rts

; IN: mv = a value: marked, if it's a cell not marked yet, and pushed on the mark stack (or the stack noted full)
mark_val:
            lda         mv
            lsr
            bcs         @no                                 ; (A fixnum)
            ldx         mv + 1
            cpx         #IMM_PAGES
            bcc         @no                                 ; (An immediate)
            lda         pk,x
            cmp         #PK_SCONS
            bcc         @no                                 ; (No cell there)
            ldy         mv                                  ; (Its mark: set, if it isn't)
            lda         mbase_lo,x
            ora         mlo,y
            sta         mp
            lda         mbase_hi,x
            sta         mp + 1
            lda         mbit,y
            and         (mp)
            bne         @no
            lda         (mp)
            ora         mbit,y
            sta         (mp)
            ldy         msp                                 ; (Pushed)
            cpy         #255
            bcs         @full
            lda         mv
            sta         ms_lo,y
            txa
            sta         ms_hi,y
            inc         msp
@no:
            rts
@full:
            lda         #1
            sta         movf
            rts

; IN: mv = a cell's reference, .X its high byte.  OUT: its mark set; C = 1 if it wasn't set before
mark_set:
            ldy         mv
            jsr         mark_addr
            and         (mp)
            bne         @was
            lda         (mp)
            ora         mbit,y
            sta         (mp)
            sec
            rts
@was:
            clc
            rts

; IN: .Y, .X = a reference (low, high).  OUT: mp = its mark's byte, .A its bit.  Keeps .X, .Y
mark_addr:
            lda         mbase_lo,x
            ora         mlo,y
            sta         mp
            lda         mbase_hi,x
            sta         mp + 1
            lda         mbit,y
            rts

; The mark stack's cells scanned, till it's empty
gc_drain:
            ldy         msp
            beq         @done
            dey
            sty         msp
            lda         ms_lo,y
            sta         mc
            lda         ms_hi,y
            sta         mc + 1
            jsr         gc_scan
            bra         gc_drain
@done:
            rts

; IN: mc = a marked cell.  Its values marked, and its spine's cell (marked now, if it wasn't) scanned in its turn.
; A cell of two values (4 bytes: a cons, a rational ...) has a way of its own, the quickest
gc_scan:
            ldx         mc + 1                              ; (Its address: deref's, here)
            ldy         pk,x
            sty         mk
            lda         bank_of,x
            sta         $00
            lda         mc
            asl
            sta         hp
            lda         ptr_hi,x
            adc         #0
            sta         hp + 1
            lda         kd_two,y
            beq         @general
            ldy         #2                                  ; (Two values: ms the second, mv the first)
            lda         (hp),y
            sta         ms
            iny
            lda         (hp),y
            sta         ms + 1
            lda         (hp)
            sta         mv
            ldy         #1
            lda         (hp),y
            sta         mv + 1
            ldy         mk
            lda         kd_two,y
            cmp         #2
            beq         @first                              ; (The first the spine: the second marked)
            bcs         @both                               ; (No spine: both marked)
            lda         mv                                  ; (The second the spine: the first marked, if it's a
            lsr                                             ;   cell: a fixnum's or an immediate's quicker)
            bcs         @spine
            lda         mv + 1
            cmp         #IMM_PAGES
            bcc         @spine
            jsr         mark_val
            bra         @spine
@first:
            lda         ms
            ldx         mv
            sta         mv
            stx         ms
            lda         ms + 1
            ldx         mv + 1
            sta         mv + 1
            stx         ms + 1
            jsr         mark_val
            bra         @spine
@both:
            jsr         mark_val
            lda         ms
            sta         mv
            lda         ms + 1
            sta         mv + 1
            jmp         mark_val
@general:
            ldx         mk
            lda         kd_size,x
            lsr
            sta         mw
            stz         mi
            stz         ms                                  ; (No spine yet: NIL)
            stz         ms + 1
@word:
            ldx         mk
            ldy         mi
            cpy         #4
            bcs         @value                              ; (Past the fourth, a value)
            lda         kd_raw,x
            and         bit_of,y
            bne         @next                               ; (Not a value)
@value:
            tya
            asl
            tay
            lda         (hp),y
            sta         mv
            iny
            lda         (hp),y
            sta         mv + 1
            lda         mi
            cmp         kd_spine,x
            bne         @mark
            lda         mv
            sta         ms
            lda         mv + 1
            sta         ms + 1
            bra         @next
@mark:
            jsr         mark_val                            ; (It doesn't change the window)
@next:
            inc         mi
            lda         mi
            cmp         mw
            bcc         @word
@spine:
            lda         ms                                  ; (The spine: a cell, marked now, scanned in its turn)
            lsr
            bcs         @done
            ldx         ms + 1
            cpx         #IMM_PAGES
            bcc         @done
            lda         pk,x
            cmp         #PK_SCONS
            bcc         @done
            ldy         ms
            lda         mbase_lo,x
            ora         mlo,y
            sta         mp
            lda         mbase_hi,x
            sta         mp + 1
            lda         mbit,y
            and         (mp)
            bne         @done                               ; (Marked already: scanned, or to be)
            lda         (mp)
            ora         mbit,y
            sta         (mp)
            sty         mc
            stx         mc + 1
            jmp         gc_scan
@done:
            rts

; Every marked cell scanned again (after the mark stack was full)
gc_rescan:
            ldx         #IMM_PAGES
@page:
            lda         pk,x
            cmp         #PK_SCONS
            bcc         @nextpage
            tay
            lda         kd_size,y
            lsr
            sta         mw                                  ; (Its references' step)
            stz         mi                                  ; (The cell's low byte)
@cell:
            ldy         mi
            jsr         mark_addr
            and         (mp)
            beq         @nextcell
            sty         mc
            stx         mc + 1
            phx
            lda         mw
            pha
            lda         mi
            pha
            jsr         gc_scan
            jsr         gc_drain
            pla
            sta         mi
            pla
            sta         mw
            plx
@nextcell:
            lda         mi
            clc
            adc         mw
            sta         mi
            bne         @cell
@nextpage:
            inx
            bne         @page
            rts

; The sweep: each kind's free list made again, of its pages' unmarked cells; a page with nothing marked free
gc_sweep:
            ldx         #PK_KINDS - 1
@lists:
            stz         fl_lo,x
            stz         fl_hi,x
            dex
            bpl         @lists
            stz         free_pages
            ldx         #IMM_PAGES
@page:
            lda         pk,x
            cmp         #PK_FREE
            bne         :+
            inc         free_pages
            bra         @nextpage
:
            cmp         #PK_SCONS
            bcc         @nextpage
            lda         mbase_lo,x                          ; (Its marks: its 16 bytes)
            sta         mp
            lda         mbase_hi,x
            sta         mp + 1
            ldy         #15
            lda         #0
@any:
            ora         (mp),y
            dey
            bpl         @any
            tay
            bne         @live
            lda         #PK_FREE                            ; (Nothing marked: free)
            sta         pk,x
            inc         free_pages
            bra         @nextpage
@live:
            phx
            jsr         sweep_page
            plx
@nextpage:
            inx
            bne         @page
            rts

; IN: .X = a page with cells marked (mp: its first mark byte).  Its unmarked cells put on its kind's free list: each
; cell's mark found in turn (mp, and mb its bit), 8 conses at once where a mark byte is full
sweep_page:
            lda         pk,x
            sta         mk
            tay
            lda         kd_size,y
            sta         hn                                  ; (A cell's size ...
            lsr
            sta         hn + 1                              ;   its references' step ...
            lsr
            sta         mw                                  ;   and its marks)
            lda         #1
            sta         mb
            stx         ht + 1
            stz         ht
            lda         #0
            jsr         deref
@cell:
            lda         mw                                  ; (Conses, at a mark byte's first: all 8 marked?)
            cmp         #1
            bne         @one
            lda         mb
            cmp         #1
            bne         @one
            lda         (mp)
            cmp         #$FF
            bne         @one
            lda         hp                                  ; (8 conses on: 32 bytes, 16 in references)
            clc
            adc         #32
            sta         hp
            bcc         :+
            inc         hp + 1
:
            inc         mp
            bne         :+
            inc         mp + 1
:
            lda         ht
            clc
            adc         #16
            sta         ht
            bne         @cell
            rts
@one:
            lda         (mp)
            and         mb
            bne         @next
            ldx         mk                                  ; (Unmarked: on the free list)
            lda         fl_lo,x
            sta         (hp)
            lda         fl_hi,x
            ldy         #1
            sta         (hp),y
            lda         ht
            sta         fl_lo,x
            lda         ht + 1
            sta         fl_hi,x
@next:
            lda         hp
            clc
            adc         hn
            sta         hp
            bcc         :+
            inc         hp + 1
:
            lda         ht
            clc
            adc         hn + 1
            sta         ht
            beq         @done                               ; (The page's end, where it wraps)
            ldx         mw                                  ; (The next cell's mark: mw bits on)
@bit:
            asl         mb
            bne         :+
            rol         mb
            inc         mp
            bne         :+
            inc         mp + 1
:
            dex
            bne         @bit
            bra         @cell
@done:
            rts

; The blobs: in each bank, those whose owners are marked (and still own them) slid down, the rest dropped
gc_blobs:
            ldx         #0
@bank:
            cpx         blob_banks
            bcs         @done
            phx
            jsr         compact
            plx
            inx
            bra         @bank
@done:
            rts

; IN: .X = a blob bank's index.  Its blobs compacted: mc the one looked at, ms where it goes
compact:
            stx         g_index
            lda         blob_bank,x
            sta         g_bank
            lda         #4
            sta         mc
            sta         ms
            lda         #$80
            sta         mc + 1
            sta         ms + 1
            lda         blob_tlo,x
            sta         g_top
            lda         blob_thi,x
            ora         #$80
            sta         g_top + 1
@blob:
            lda         mc + 1                              ; (To the top)
            cmp         g_top + 1
            bcc         @one
            beq         :+
            jmp         @end
:
            lda         mc
            cmp         g_top
            bcc         @one
            jmp         @end
@one:
            lda         g_bank
            sta         $00
            lda         (mc)                                ; (Its owner, its size)
            sta         mv
            ldy         #1
            lda         (mc),y
            sta         mv + 1
            iny
            lda         (mc),y
            clc
            adc         #5
            and         #$FE
            sta         g_size
            iny
            lda         (mc),y
            adc         #0
            sta         g_size + 1
            jsr         blob_ref                            ; (Its reference: hb)
            jsr         blob_live
            bcc         @skip
            lda         ms                                  ; (Where it is already?)
            cmp         mc
            bne         @move
            lda         ms + 1
            cmp         mc + 1
            beq         @kept
@move:
            jsr         blob_move
            lda         ms                                  ; (Its new reference, in its owner)
            sta         hb
            lda         ms + 1
            and         #$1F
            lsr
            ror         hb
            sta         hb + 1
            lda         g_index
            asl
            asl
            asl
            asl
            ora         hb + 1
            sta         hb + 1
            lda         mv
            ldx         mv + 1
            jsr         deref
            lda         g_word
            asl
            tay
            lda         hb
            sta         (hp),y
            iny
            lda         hb + 1
            sta         (hp),y
@kept:
            lda         ms
            clc
            adc         g_size
            sta         ms
            lda         ms + 1
            adc         g_size + 1
            sta         ms + 1
@skip:
            lda         mc
            clc
            adc         g_size
            sta         mc
            lda         mc + 1
            adc         g_size + 1
            sta         mc + 1
            jmp         @blob
@end:
            ldx         g_index                             ; (The bank's new top: $2000 when it's full, every blob
            lda         ms                                  ;   in it live, so less $80, not and #$1F, which made
            sta         blob_tlo,x                          ;   that 0, the bank seeming empty)
            lda         ms + 1
            sec
            sbc         #$80
            sta         blob_thi,x
            rts

; OUT: hb = the reference of the blob at mc (in bank g_index)
blob_ref:
            lda         mc
            sta         hb
            lda         mc + 1
            and         #$1F
            lsr
            ror         hb
            sta         hb + 1
            lda         g_index
            asl
            asl
            asl
            asl
            ora         hb + 1
            sta         hb + 1
            rts

; IN: mv = a blob's owner, hb = the blob.  OUT: C = 1 if the owner is a marked cell whose blob word (g_word) is hb
blob_live:
            lda         mv
            lsr
            bcs         @dead
            ldx         mv + 1
            cpx         #IMM_PAGES
            bcc         @dead
            lda         pk,x
            cmp         #PK_SCONS
            bcc         @dead
            tay
            lda         kd_blob,y
            bmi         @dead                               ; (A kind with no blob: $FF)
            sta         g_word
            ldy         mv
            jsr         mark_addr
            and         (mp)
            beq         @dead
            lda         mv
            jsr         deref                               ; (The owner's word: its bank in the window, for now)
            lda         g_word
            asl
            tay
            lda         (hp),y
            cmp         hb
            bne         @dead
            iny
            lda         (hp),y
            cmp         hb + 1
            bne         @dead
            sec
            rts
@dead:
            clc
            rts

; The blob at mc moved down to ms, g_size bytes, in bank g_bank
blob_move:
            lda         g_bank
            sta         $00
            lda         mc
            sta         hq
            lda         mc + 1
            sta         hq + 1
            lda         ms
            sta         hp
            lda         ms + 1
            sta         hp + 1
            ldy         #0
            ldx         g_size + 1
            beq         @last
@pages:
            lda         (hq),y
            sta         (hp),y
            iny
            bne         @pages
            inc         hq + 1
            inc         hp + 1
            dex
            bne         @pages
@last:
            ldx         g_size
            beq         @done
@bytes:
            lda         (hq),y
            sta         (hp),y
            iny
            dex
            bne         @bytes
@done:
            rts

.ifdef HL_DATA4
.segment "DATA4"
.else
.segment "DATA"
.endif

; The roots that aren't the heap's own: gc_hook's (an rts, till the evaluator has some).  Each marked with mark_val
; (gc_collect's; in the RAM, so the evaluator can set gc_hook)
gc_roots:
            jmp         (gc_hook)
gc_none:
            rts
gc_hook:    .word       gc_none

; ---- The free space (for the tests: slow)

; OUT: hn = the cell heap's free room in 4-byte units: its free pages' and its free lists' cells
heap_free:
            stz         hn
            lda         free_pages                          ; (A page: 128 units)
            lsr
            sta         hn + 1
            ror         hn
            ldx         #PK_SCONS
@kind:
            lda         fl_lo,x
            sta         ht
            lda         fl_hi,x
            sta         ht + 1
@cell:
            lda         ht
            ora         ht + 1
            beq         @nextkind
            lda         kd_size,x                           ; (Its units)
            lsr
            lsr
            clc
            adc         hn
            sta         hn
            bcc         :+
            inc         hn + 1
:
            phx
            lda         ht
            ldx         ht + 1
            jsr         deref
            plx
            lda         (hp)
            sta         ht
            ldy         #1
            lda         (hp),y
            sta         ht + 1
            bra         @cell
@nextkind:
            inx
            cpx         #PK_KINDS
            bcc         @kind
            rts

; ---- The kinds of cell: each one's size, its words that aren't values (a bit each, words 0-3: a frame's past the
; fourth are all values), the word that's its blob ($FF: none), and its spine ($FF: none)
kd_size:    .byte       0, 0, 0                             ; (PK_NONE, PK_IMM, PK_FREE)
            .byte       4, 4, 4, 4, 4, 8, 4, 4, 4           ; (PK_SCONS to PK_ERROR)
            .byte       4, 8, 8, 16, 8, 8                   ; (PK_PARTIAL to PK_STREAM)
            .byte       8, 16, 32, 64                       ; (The frames)
            .byte       4                                   ; (PK_BUFFER)
kd_raw:     .byte       0, 0, 0
            .byte       %0000, %0000, %0000, %0011, %0011, %1111, %0000, %0000, %0000
            .byte       %0000, %1001, %1001, %1000, %1000, %1111
            .byte       %0000, %0000, %0000, %0000
            .byte       %0011
kd_blob:    .byte       $FF, $FF, $FF
            .byte       $FF, $FF, $FF, 1, 1, $FF, $FF, $FF, $FF
            .byte       $FF, 0, 0, $FF, $FF, $FF
            .byte       $FF, $FF, $FF, $FF
            .byte       1
kd_spine:   .byte       $FF, $FF, $FF
            .byte       1, 1, 0, $FF, $FF, $FF, $FF, $FF, $FF
            .byte       1, 2, 2, 1, $FF, $FF
            .byte       0, 0, 0, 0
            .byte       $FF
; ... and, for a cell of two values (4 bytes), the collector's quickest way (0: the general): 1, the second the spine
; (a cons); 2, the first (a scoped list); 3, no spine
kd_two:     .byte       0, 0, 0
            .byte       1, 1, 2, 0, 0, 0, 3, 3, 3
            .byte       1, 0, 0, 0, 0, 0
            .byte       0, 0, 0, 0
            .byte       0

bit_of:     .byte       1, 2, 4, 8, 16, 32, 64, 128

.code
