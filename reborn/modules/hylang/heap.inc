; ****************************************************************************
; heap.inc - hylang's heap (docs/hylang.md; the plan's §17.3; spike S5, step 7.1): 16-bit values, objects in the
; task's own RAM banks, and a mark-and-sweep collector that never moves them (native code may hold their addresses).
;   A value: bit 0 set, a fixnum (the 15 bits above it, signed: -16384 to 16383); clear, an object, whose high
; byte is its page (0-255, of the heap's 64K: HEAP_BANKS banks of 32 pages) and low byte its place in the page.
; page_bank and page_addr make a page its bank (for RAM_BANK) and its address (in the bank window, $8000-$9FFF);
; page_kind says what a page holds, so a value's kind is a look-up of its high byte.  Page 0 is the constants (NIL
; is $0000, T $0002) and pages 1 and 2 the characters ($0100 + 2c): values with no memory behind them.
;   A page of cells holds one kind (BIBOP): pairs, 4 bytes (the car, the cdr: no header), or cells of 8, 16, 32, 64
; or 128 bytes, each a type byte (OT_*), a length byte, then the rest.  So no object crosses a page, or a bank.
;   The collector: the marks (a bit for each 4 bytes of the heap, in the task's RAM: 8 tables of 256, table n being
; each page's bytes 32n to 32n+31; mark_tab and mark_bit find a place's) cleared; the roots (h_a and h_d, then
; roots_lo/hi) marked; each marked object scanned from the mark stack (a bank of its own: 4096 values), a list's run
; of cdrs in place, so a list costs no stack.  A full stack is noted (mark_over), and then the marked cells are
; scanned again till nothing new is marked.  Then a page with nothing marked is free; the others are swept lazily.
;   Its code and tables are in the task's RAM (the module's DATA), where both of hylang's banks see them.
;   Allocation: each kind fills a page in order (cur_page, cur_off): a page that was free, every cell taken in turn,
; or, after a collection, one of its pages with cells live (page_sweep), its unmarked cells taken in turn (the lazy
; sweep: nothing's linked, and a collection's pause is its marking); then a free page; then a collection.

HEAP_BANKS      = 8             ; The heap: 64K, 256 pages
ROOTS_MAX       = 64            ; The values held for the collector (roots_lo/hi)
MARK_STACK      = $8000         ; The mark stack, in mark_bank (to $9FFF)

NIL             = $0000         ; The constants (page 0) ...
T_VAL           = $0002
CHARS           = $0100         ;   and the characters: $0100 + 2c (pages 1 and 2)

; Page kinds (page_kind)
PK_FREE         = 0             ; Free
PK_CONST        = 1             ; Page 0: the constants
PK_CHAR         = 2             ; Pages 1 and 2: the characters
PK_PAIR         = 3             ; Pairs (4 bytes: the car, the cdr) ...
PK_C8           = 4             ;   and cells of 8 bytes (a type, a length, the rest) ...
PK_C16          = 5
PK_C32          = 6
PK_C64          = 7
PK_C128         = 8             ;   ... to 128
PK_KINDS        = 9

; An object's type (a cell's first byte).  Its references (the values the collector follows) come after the type
; and the byte after it, and ot_refs says how many (for a vector: its length)
OT_STRING       = 1             ; Its length, then its bytes
OT_VECTOR       = 2             ; Its length in values, then the values
OT_QEXPR        = 3             ; Its list (a chain of pairs), and the scope it was written in (NIL: none yet)
OT_SEXPR        = 4             ; Its list, and the scope it was written in (NIL: none; an fexpr's argument has one)
OT_SYMBOL       = 5             ; Its name's length; the next in its bucket, its global value; its name
OT_ATOM         = 6             ; Its name's length; the next in its bucket, NIL; its name
OT_FUNC         = 7             ; Its flags; its formals, body, closure (a frame), and the arguments bound so far
OT_ERROR        = 8             ; Its message (a string), its code (an atom, or NIL)
OT_FRAME        = 9             ; Its parent (a frame, or GLOBAL), its bindings (a list: a symbol, its value, ...)
OT_SYMREF       = 10            ; A symbol, and the scope it was written in (an fexpr's argument)
OT_BIG          = 11            ; An integer past a fixnum: its length (bytes; $80: negative), its bytes, the low first
OT_RAT          = 12            ; A rational: its numerator, its denominator (integers)
OT_FIX          = 13            ; A fixed decimal: its digits (an integer), its places (a fixnum)
OT_COMPLEX      = 14            ; A complex number: its real part, its imaginary part (each a number, not complex)
OT_LSTR         = 15            ; A string of more than 126 bytes: its cells (a chain of OT_STRING), its length
OT_HASH         = 16            ; A hash: its entries (a chain of lists: a key, its value, its tags), its tags (a
                                ; chain of atoms); its second byte HF_PROXY: a method's &0, the hash it stands for
OT_PBUILT       = 17            ; A built-in partially applied: the built-in, the values given it so far (a chain)
OT_TYPES        = 18

.zeropage
h_v:        .res        2                                   ; A value, in and out
h_a:        .res        2                                   ; cons's car and cdr (roots while it makes the pair)
h_d:        .res        2
h_p:        .res        2                                   ; An object's address (deref's: its bank selected)
h_t:        .res        2                                   ; The collector's object at hand
h_c:        .res        2                                   ; The collector's child at hand
h_m:        .res        2                                   ; A mark's byte's table (marks + 256n)
h_sp:       .res        2                                   ; The mark stack's top
h_k:        .res        1                                   ; A kind
h_n:        .res        1                                   ; A count, or a page
h_f:        .res        1                                   ; A flag

.bss
heap_bank:  .res        1                                   ; The heap's first bank (page 0's) ...
mark_bank:  .res        1                                   ;   and the mark stack's, after its last
page_kind:  .res        256                                 ; Each page's kind (PK_*) ...
page_bank:  .res        256                                 ;   its bank ...
page_addr:  .res        256                                 ;   its address's high byte ($80-$9F) ...
page_sweep: .res        256                                 ;   and not 0 for cells live at the last collection
marks:      .res        2048                                ; The marks: table n, page p, its units 8n-8n+7
cur_page:   .res        PK_KINDS                            ; Each kind's page being filled (0: none) ...
cur_off:    .res        PK_KINDS                            ;   its next cell there ...
cur_fresh:  .res        PK_KINDS                            ;   1 if it was free (else: take its unmarked cells) ...
sweep_at:   .res        PK_KINDS                            ;   and where its next page to sweep is looked for (0: none)
page_hint:  .res        1                                   ; Where a free page is looked for from
roots_lo:   .res        ROOTS_MAX                           ; The roots (root_push)
roots_hi:   .res        ROOTS_MAX
root_n:     .res        1
mark_over:  .res        1                                   ; The mark stack was full: a rescan's needed
rs_page:    .res        1                                   ; The rescan's place
rs_off:     .res        1
rs_kind:    .res        1
gc_count:   .res        2                                   ; Collections made
gc_rescans: .res        1                                   ; Rescans made (the mark stack full)
gc_hook:    .res        2                                   ; The program's roots: a routine that marks them (mark_root)
.segment "DATA"

; ---- Making it

; The heap made: HEAP_BANKS + 1 of the task's banks (BANKS_ALLOC), the page tables, every page free but the
; constants' and the characters'.  OUT: C = 0; or C = 1, .A = the error (E_NOMEM)
heap_init:
            lda         #HEAP_BANKS + 1
            jsr         BANKS_ALLOC
            bcs         @done
            sta         heap_bank
            clc
            adc         #HEAP_BANKS
            sta         mark_bank
            ldx         #0
@page:
            txa                                             ; (The bank: heap_bank + the page / 32)
            lsr
            lsr
            lsr
            lsr
            lsr
            clc
            adc         heap_bank
            sta         page_bank,x
            txa                                             ; (Its address: $80 + the page % 32)
            and         #$1F
            ora         #$80
            sta         page_addr,x
            stz         page_kind,x
            stz         page_sweep,x
            inx
            bne         @page
            lda         #PK_CONST
            sta         page_kind
            lda         #PK_CHAR
            sta         page_kind + 1
            sta         page_kind + 2
            lda         #<marks                             ; (A mark's table: its low byte, always)
            sta         h_m
            jsr         forget_cur
            stz         root_n
            stz         gc_count
            stz         gc_count + 1
            stz         gc_rescans
            LDR         gc_hook, gc_no_hook
            clc
@done:
            rts

; Every kind's page being filled forgotten, its sweep from the first page that's anyone's
forget_cur:
            ldx         #PK_KINDS - 1
            lda         #3
:
            stz         cur_page,x
            sta         sweep_at,x
            dex
            bpl         :-
            sta         page_hint
            rts

; ---- Values and objects

; h_v's object: its bank selected, h_p its address.  Keeps .Y
deref:
            ldx         h_v + 1
            lda         page_bank,x
            sta         RAM_BANK
            lda         page_addr,x
            sta         h_p + 1
            lda         h_v
            sta         h_p
            rts

; h_v's kind: .A = PK_* (a fixnum: PK_FREE, which no object is)
kind_of:
            lda         h_v
            lsr
            lda         #PK_FREE
            bcs         :+
            ldx         h_v + 1
            lda         page_kind,x
:
            rts

; car and cdr: h_v (a pair) = its car, or its cdr (each with deref's code: they're the most used)
car:
            ldx         h_v + 1
            lda         page_bank,x
            sta         RAM_BANK
            lda         page_addr,x
            sta         h_p + 1
            lda         h_v
            sta         h_p
            ldy         #1
            lda         (h_p),y
            tax
            lda         (h_p)
            sta         h_v
            stx         h_v + 1
            rts

cdr:
            ldx         h_v + 1
            lda         page_bank,x
            sta         RAM_BANK
            lda         page_addr,x
            sta         h_p + 1
            lda         h_v
            sta         h_p
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            rts

; A pair of h_a (its car) and h_d (its cdr), which are roots meanwhile.  OUT: C = 0, h_v = it (h_p its address,
; its bank selected); or C = 1: no room, even after a collection
cons:
            ldx         cur_page + PK_PAIR                  ; (A page that was free: its next cell)
            beq         @slow
            lda         cur_fresh + PK_PAIR
            beq         @slow
            stx         h_v + 1
            lda         page_bank,x
            sta         RAM_BANK
            lda         page_addr,x
            sta         h_p + 1
            lda         cur_off + PK_PAIR
            sta         h_v
            sta         h_p
            clc
            adc         #4
            sta         cur_off + PK_PAIR
            bcc         :+
            stz         cur_page + PK_PAIR                  ; (Its end)
:
            ldy         #1
@fill:
            lda         h_a
            sta         (h_p)
            lda         h_a + 1
            sta         (h_p),y
            iny
            lda         h_d
            sta         (h_p),y
            iny
            lda         h_d + 1
            sta         (h_p),y
            clc
            rts
@slow:
            ldy         #PK_PAIR
            jsr         alloc
            bcs         :+
            ldy         #1
            bra         @fill
:
            rts

; A cell of kind .Y (PK_PAIR, PK_C8 ...).  The values the caller holds must be roots, or in h_a and h_d,
; meanwhile: it may collect.  OUT: C = 0, h_v = it, h_p its address (its bank selected); or C = 1: no room
alloc:
            sty         h_k
            stz         h_f                                 ; (No collection yet)
@again:
            ldy         h_k
            ldx         cur_page,y
            beq         @page
            lda         cur_fresh,y
            bne         @take
            stx         h_n                                 ; ---- A page swept: its next unmarked cell
@look:
            ldx         cur_off,y
            lda         mark_tab,x
            sta         h_m + 1
            lda         mark_bit,x
            ldy         h_n
            and         (h_m),y
            beq         @free
            ldy         h_k                                 ; (Live: the next)
            txa
            clc
            adc         kind_size,y
            sta         cur_off,y
            bcc         @look
            lda         #0                                  ; (Its end)
            sta         cur_page,y
            bra         @page
@free:
            ldy         h_k
            ldx         h_n
@take:
            stx         h_v + 1                             ; ---- The cell at cur_off
            lda         page_bank,x
            sta         RAM_BANK
            lda         page_addr,x
            sta         h_p + 1
            lda         cur_off,y
            sta         h_v
            sta         h_p
            clc
            adc         kind_size,y
            sta         cur_off,y
            bcc         :+
            lda         #0                                  ; (Its end)
            sta         cur_page,y
:
            clc
            rts
@page:
            jsr         next_page                           ; ---- Another page: to sweep, a free one, or a
            bcc         @again                              ; collection's
            jsr         new_page
            bcc         @again
            lda         h_f
            bne         @full
            inc         h_f
            jsr         gc
            bra         @again
@full:
            sec
            rts

; Kind h_k's next page with cells live at the last collection, to sweep, made its page being filled.  OUT: C = 0;
; or C = 1, none left
next_page:
            ldy         h_k
            ldx         sweep_at,y
            beq         @none
@look:
            lda         page_kind,x
            cmp         h_k
            bne         @next
            lda         page_sweep,x
            bne         @found
@next:
            inx
            bne         @look
            lda         #0                                  ; (None left till the next collection)
            sta         sweep_at,y
@none:
            sec
            rts
@found:
            stz         page_sweep,x
            txa
            sta         cur_page,y
            inc         a                                   ; (Its next: past the last, none left)
            sta         sweep_at,y
            lda         #0
            sta         cur_off,y
            sta         cur_fresh,y
            clc
            rts

; A free page made kind h_k's page being filled.  OUT: C = 0; or C = 1, none free
new_page:
            ldx         page_hint
            ldy         #0                                  ; (Pages looked at: all 256 at most)
@look:
            lda         page_kind,x
            beq         @found
            inx
            bne         :+
            ldx         #3                                  ; (Past the last: from the first that's anyone's)
:
            dey
            bne         @look
            sec
            rts
@found:
            lda         h_k
            sta         page_kind,x
            stz         page_sweep,x
            ldy         h_k
            txa
            sta         cur_page,y
            lda         #0
            sta         cur_off,y
            lda         #1
            sta         cur_fresh,y
            inx
            stx         page_hint
            clc
            rts

; A mark's place: .Y = a page, .A = a place in it.  OUT: (h_m),y its byte, .A its bit.  Keeps .Y
bit_of:
            tax
            lda         mark_tab,x
            sta         h_m + 1
            lda         mark_bit,x
            rts

; ---- The roots

; h_v held for the collector (with root_set, root_get, root_drop).  OUT: C = 1, the roots are full
root_push:
            ldx         root_n
            cpx         #ROOTS_MAX
            bcs         :+
            lda         h_v
            sta         roots_lo,x
            lda         h_v + 1
            sta         roots_hi,x
            inc         root_n
            clc
:
            rts

; The last root = h_v
root_set:
            ldx         root_n
            lda         h_v
            sta         roots_lo - 1,x
            lda         h_v + 1
            sta         roots_hi - 1,x
            rts

; h_v = the last root
root_get:
            ldx         root_n
            lda         roots_lo - 1,x
            sta         h_v
            lda         roots_hi - 1,x
            sta         h_v + 1
            rts

; The last root let go
root_drop:
            dec         root_n
            rts

; ---- The collector

; A collection: the live objects marked, from the roots, h_a and h_d; then the pages with nothing live are free,
; and the rest are swept lazily, as their cells are wanted (alloc)
gc:
            ldx         #0                                  ; ---- The marks cleared
:
            stz         marks,x
            stz         marks + $100,x
            stz         marks + $200,x
            stz         marks + $300,x
            stz         marks + $400,x
            stz         marks + $500,x
            stz         marks + $600,x
            stz         marks + $700,x
            inx
            bne         :-
            jsr         forget_cur
            LDR         h_sp, MARK_STACK                    ; ---- The roots marked
            stz         mark_over
            MOVR        h_c, h_a
            jsr         mark_root
            MOVR        h_c, h_d
            jsr         mark_root
            jsr         gc_more_roots                       ; (The program's: symbols, its stack ...)
            ldx         #0
@root:
            cpx         root_n
            bcs         @drain
            lda         roots_lo,x
            sta         h_c
            lda         roots_hi,x
            sta         h_c + 1
            phx
            jsr         mark_root
            plx
            inx
            bra         @root
@drain:
            jsr         drain                               ; ---- What they reach marked
            lda         mark_over
            beq         @pages
            jsr         rescan
            bra         @drain
@pages:
            ldx         #3                                  ; ---- Each page of cells: free, or to sweep
@page:
            lda         page_kind,x
            cmp         #PK_PAIR
            bcc         @next
            lda         marks,x
            ora         marks + $100,x
            ora         marks + $200,x
            ora         marks + $300,x
            ora         marks + $400,x
            ora         marks + $500,x
            ora         marks + $600,x
            ora         marks + $700,x
            sta         page_sweep,x                        ; (Not 0: some live)
            bne         @next
            stz         page_kind,x
@next:
            inx
            bne         @page
            inc         gc_count
            bne         :+
            inc         gc_count + 1
:
            rts

; The program's roots marked (gc_hook: it sets h_c to each and calls mark_root)
gc_more_roots:
            jmp         (gc_hook)

gc_no_hook:
            rts

; h_c marked, and pushed to be scanned, if it's an object not marked yet
mark_root:
            jsr         mark_set
            bcc         :+
            jmp         push
:
            rts

; h_c marked, if it's an object not marked yet.  OUT: C = 1, it was (it's to be scanned); C = 0, not
mark_set:
            lda         h_c
            lsr
            bcs         @no                                 ; (A fixnum)
            ldx         h_c + 1
            lda         page_kind,x
            cmp         #PK_PAIR
            bcc         @no                                 ; (A constant or a character: C = 0)
            ldx         h_c
            lda         mark_tab,x
            sta         h_m + 1
            ldy         h_c + 1
            lda         mark_bit,x
            and         (h_m),y
            bne         @no
            lda         mark_bit,x
            ora         (h_m),y
            sta         (h_m),y
            sec
            rts
@no:
            clc
            rts

; h_c onto the mark stack; if it's full, mark_over set (a rescan scans what's marked).  Changes RAM_BANK
push:
            lda         mark_bank
            sta         RAM_BANK
            lda         h_sp + 1
            cmp         #>(MARK_STACK + $2000)
            bcs         @full
            lda         h_c
            sta         (h_sp)
            ldy         #1
            lda         h_c + 1
            sta         (h_sp),y
            clc
            lda         h_sp
            adc         #2
            sta         h_sp
            bcc         :+
            inc         h_sp + 1
:
            rts
@full:
            lda         #1
            sta         mark_over
            rts

; The mark stack's objects scanned (and what they push), till it's empty
drain:
            lda         h_sp
            bne         @pop
            lda         h_sp + 1
            cmp         #>MARK_STACK
            beq         @done
@pop:
            lda         mark_bank
            sta         RAM_BANK
            sec
            lda         h_sp
            sbc         #2
            sta         h_sp
            bcs         :+
            dec         h_sp + 1
:
            lda         (h_sp)
            sta         h_t
            ldy         #1
            lda         (h_sp),y
            sta         h_t + 1
            jsr         scan
            bra         drain
@done:
            rts

; h_t's children marked (a marked object's): a pair's car pushed and its cdr scanned in place, a vector's values
; pushed.  Changes h_t
scan:
            ldx         h_t + 1
            lda         page_kind,x
            cmp         #PK_PAIR
            bne         @cell
@pair:
            lda         page_bank,x                         ; ---- A pair (.X its page)
            sta         RAM_BANK
            lda         page_addr,x
            sta         h_p + 1
            lda         h_t
            sta         h_p
            lda         (h_p)                               ; (The car ...
            sta         h_c
            ldy         #1
            lda         (h_p),y
            sta         h_c + 1
            iny                                             ;   and the cdr, before a push changes the bank)
            lda         (h_p),y
            sta         h_t
            iny
            lda         (h_p),y
            sta         h_t + 1
            lda         h_c                                 ; (The car, marked and pushed: not a fixnum, a
            lsr                                             ; constant or a character, pages 0-2)
            bcs         @cdr
            lda         h_c + 1
            cmp         #3
            bcc         @cdr
            jsr         mark_root
@cdr:
            lda         h_t                                 ; (The cdr, marked and scanned here, if it's an
            lsr                                             ; object not marked yet)
            bcs         @done
            ldy         h_t + 1
            cpy         #3
            bcc         @done
            ldx         h_t
            lda         mark_tab,x
            sta         h_m + 1
            lda         mark_bit,x
            and         (h_m),y
            bne         @done
            lda         mark_bit,x
            ora         (h_m),y
            sta         (h_m),y
            ldx         h_t + 1
            lda         page_kind,x
            cmp         #PK_PAIR
            beq         @pair
@cell:
            lda         page_bank,x                         ; ---- A cell: its references (.X its page), from its
            sta         RAM_BANK                            ; third byte on, as many as its type has (a vector: its
            lda         page_addr,x                         ; length)
            sta         h_p + 1
            lda         h_t
            sta         h_p
            lda         (h_p)
            tax
            lda         ot_refs,x
            cmp         #$FF
            bne         :+
            ldy         #1
            lda         (h_p),y
:
            sta         h_n
            ldy         #2
@value:
            lda         h_n
            beq         @done
            ldx         h_t + 1                             ; (Its bank again: a push changes it)
            lda         page_bank,x
            sta         RAM_BANK
            lda         (h_p),y
            sta         h_c
            iny
            lda         (h_p),y
            sta         h_c + 1
            iny
            phy
            jsr         mark_root
            ply
            dec         h_n
            bra         @value
@done:
            rts

; The mark stack overflowed: every marked cell's children marked again (those not marked yet pushed, and the
; stack drained after each cell), from the first page to the last.  A cell marked meanwhile is found too, or
; pushed; if the stack fills again, mark_over is set again, and gc rescans again
rescan:
            inc         gc_rescans
            stz         mark_over
            lda         #3
            sta         rs_page
@page:
            ldx         rs_page
            lda         page_kind,x
            cmp         #PK_PAIR
            bcc         @nextp
            sta         rs_kind
            stz         rs_off
@cell:
            ldy         rs_page
            lda         rs_off
            jsr         bit_of
            and         (h_m),y
            beq         @nextc
            lda         rs_off
            sta         h_t
            lda         rs_page
            sta         h_t + 1
            jsr         scan
            jsr         drain
@nextc:
            ldx         rs_kind
            clc
            lda         rs_off
            adc         kind_size,x
            sta         rs_off
            bcc         @cell
@nextp:
            inc         rs_page
            bne         @page
            rts

; ---- Strings and vectors (cells with a header: a type, a length, the rest)

; The kind of cell for .A bytes (2 to 128).  OUT: .Y = PK_C8 ...; C = 1, too many
cell_kind:
            ldy         #PK_C8
            cmp         #9
            bcc         @ok
            iny
            cmp         #17
            bcc         @ok
            iny
            cmp         #33
            bcc         @ok
            iny
            cmp         #65
            bcc         @ok
            iny
            cmp         #129
            bcs         @big
@ok:
            clc
            rts
@big:
            sec
            rts

; A string of .A bytes (126 at most) from r0 (a buffer in the task's RAM).  OUT: C = 0, h_v = it; or C = 1
make_string:
            cmp         #127
            bcs         @no
            pha                                             ; (Kept: alloc may collect, which uses h_n)
            clc
            adc         #2
            jsr         cell_kind
            jsr         alloc
            pla
            sta         h_n
            bcs         @no
            lda         #OT_STRING
            sta         (h_p)
            ldy         #1
            lda         h_n
            sta         (h_p),y
            ldy         #0
@byte:
            cpy         h_n
            beq         @done
            lda         (r0),y
            iny
            iny
            sta         (h_p),y
            dey
            bra         @byte
@done:
            clc
            rts
@no:
            sec
            rts

; A vector of .A values (63 at most), each NIL.  OUT: C = 0, h_v = it; or C = 1
make_vector:
            cmp         #64
            bcs         @no
            pha                                             ; (Kept: alloc may collect, which uses h_n)
            asl
            adc         #2
            jsr         cell_kind
            jsr         alloc
            pla
            sta         h_n
            bcs         @no
            lda         #OT_VECTOR
            sta         (h_p)
            ldy         #1
            lda         h_n
            sta         (h_p),y
            asl
            beq         @done
            tax
            lda         #0
@nil:
            iny
            sta         (h_p),y
            dex
            bne         @nil
@done:
            clc
            rts
@no:
            sec
            rts

; h_v (a vector)'s value .Y (0 up) = h_a
vector_set:
            tya
            asl
            tay
            iny
            iny
            jsr         deref
            lda         h_a
            sta         (h_p),y
            iny
            lda         h_a + 1
            sta         (h_p),y
            rts

; h_v (a vector) = its value .Y (0 up)
vector_ref:
            tya
            asl
            tay
            iny
            iny
            jsr         deref
            lda         (h_p),y
            tax
            iny
            lda         (h_p),y
            stx         h_v
            sta         h_v + 1
            rts

; The free pages: .A = how many
free_pages:
            ldx         #0
            ldy         #0
:
            lda         page_kind,x
            bne         :+
            iny
:
            inx
            bne         :--
            tya
            rts

.segment "DATA"
kind_size:  .byte       0, 0, 0, 4, 8, 16, 32, 64, 128

; Each type's references (OT_*: $FF, a vector's length)
ot_refs:    .byte       0, 0, $FF, 2, 2, 2, 2, 4, 2, 2, 2, 0, 2, 2, 2, 2, 2, 2

; A place in a page: its mark's table (the high byte of marks + 256 (place / 32)), and its bit
mark_tab:
.repeat 256, I
            .byte       .hibyte(marks) + (I >> 5)
.endrepeat
mark_bit:
.repeat 256, I
            .byte       1 << ((I >> 2) & 7)
.endrepeat
.code
