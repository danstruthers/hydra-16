; ****************************************************************************
; t_heap - hylang's heap (modules/hylang/heap.inc: phase 7, spike S5), run as init: values and their kinds, pairs,
; strings and vectors, and the collector (a list kept while garbage is taken back, a mark stack that overflows, the
; heap filled and emptied).  The times, between marks: 1000 pairs made in a fresh heap ("<cons" "cons>") and from
; swept pages ("<cons2" "cons2>"); a collection with 1000 pairs live ("<gc1k" "gc1k>") and with 9000 ("<gc9k").

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_heap", main

; A fixnum's value
.define FIX(n) ((n) * 2 + 1)

.zeropage
i:          .res        2                                   ; A count
sum:        .res        2                                   ; A list's values, added up (16 bits)
len:        .res        2                                   ; A list's length
v:          .res        2                                   ; A value the test keeps
w:          .res        2

.code

main:
            stz         T_FAILS
            jsr         heap_init
            EXPECT_OK   "heap_init: the heap's 8 banks and the mark stack's"
            jsr         free_pages
            EXPECT_A    253, "253 pages free (the constants' and the characters' aren't)"

; ---- Values and their kinds
            LDR         h_v, NIL
            jsr         kind_of
            EXPECT_A    PK_CONST, "NIL: a constant"
            LDR         h_v, CHARS + 2 * 'A'
            jsr         kind_of
            EXPECT_A    PK_CHAR, "A character"
            LDR         h_v, FIX(-5)
            jsr         kind_of
            EXPECT_A    PK_FREE, "A fixnum: no object"
            LDR         h_a, FIX(1)
            LDR         h_d, NIL
            jsr         cons
            EXPECT_OK   "cons"
            jsr         kind_of
            EXPECT_A    PK_PAIR, "A pair"
            MOVR        v, h_v
            jsr         car
            lda         h_v
            EXPECT_A    <FIX(1), "Its car"
            MOVR        h_v, v
            jsr         cdr
            lda         h_v
            ora         h_v + 1
            EXPECT_A    0, "Its cdr: NIL"

; ---- 1000 pairs made in a fresh heap (timed), then a list of 1000 kept
            LDR         h_a, FIX(7)
            LDR         h_d, NIL
            LDR         i, 1000
            MARK        "<cons"
@cons:
            jsr         cons
            bcs         @consfail
            dec         i
            bne         @cons
            dec         i + 1
            bpl         @cons
@consfail:
            php
            MARK        "cons>"
            jsr         t_crlf
            plp
            EXPECT_OK   "1000 pairs made"
            LDR         i, 1000
            jsr         build
            EXPECT_OK   "A list of 1000, kept (a root)"
            jsr         check
            EXPECT_OK   "Its length, 1000; its values, 0-999"

; ---- A collection with the list live (timed); the 1000 pairs that weren't kept taken back
            MARK        "<gc1k"
            jsr         gc
            MARK        "gc1k>"
            jsr         t_crlf
            jsr         check
            EXPECT_OK   "The list, after a collection"
            jsr         free_pages
            cmp         #253 - 16 - 2
            bcs         :+
            NOTOK       "1000 pairs not kept taken back: their pages free"
            bra         :++
:
            OK          "1000 pairs not kept taken back: their pages free"
:

; ---- Garbage: 30,000 pairs not kept, while the list is (the heap holds 16,000: it collects as it goes)
            lda         gc_count
            sta         w
            LDR         i, 30000
            LDR         h_a, FIX(3)
            LDR         h_d, NIL
@garbage:
            jsr         cons
            bcs         @garbagefail
            dec         i
            bne         @garbage
            dec         i + 1
            bpl         @garbage
            clc
@garbagefail:
            EXPECT_OK   "30,000 pairs not kept"
            lda         gc_count
            sec
            sbc         w
            cmp         #1
            bcs         :+
            NOTOK       "Collections made as the heap filled"
            bra         :++
:
            OK          "Collections made as the heap filled"
:
            jsr         check
            EXPECT_OK   "The list, after them"

; ---- Pages half live: a list of 1000 made with a pair not kept between each; then 1000 from the swept pages
            jsr         root_drop
            jsr         gc
            LDR         i, 1000
            jsr         build_combed
            EXPECT_OK   "A list of 1000 made with a pair not kept between each"
            jsr         gc
            jsr         check
            EXPECT_OK   "It, after a collection"
            LDR         h_a, FIX(9)
            LDR         h_d, NIL
            LDR         i, 1000
            MARK        "<cons2"
@cons2:
            jsr         cons
            bcs         @cons2fail
            dec         i
            bne         @cons2
            dec         i + 1
            bpl         @cons2
@cons2fail:
            php
            MARK        "cons2>"
            jsr         t_crlf
            plp
            EXPECT_OK   "1000 pairs made from swept pages"
            jsr         check
            EXPECT_OK   "The list, still"

; ---- A collection with 9000 pairs live (timed)
            LDR         i, 8000
            jsr         build
            EXPECT_OK   "A list of 8000 more, kept"
            MARK        "<gc9k"
            jsr         gc
            MARK        "gc9k>"
            jsr         t_crlf
            jsr         check
            EXPECT_OK   "It, after a collection"
            jsr         root_drop
            jsr         check
            EXPECT_OK   "The first list, too"
            jsr         root_drop

; ---- Strings and vectors
            jsr         gc
            LDR         r0, s_hello
            lda         #5
            jsr         make_string
            EXPECT_OK   "A string of 5"
            jsr         kind_of
            EXPECT_A    PK_C8, "In a cell of 8"
            jsr         root_push
            LDR         r0, s_long
            lda         #100
            jsr         make_string
            EXPECT_OK   "A string of 100"
            jsr         kind_of
            EXPECT_A    PK_C128, "In a cell of 128"
            jsr         root_push
            lda         #3
            jsr         make_vector                         ; (It may collect: the strings are roots)
            EXPECT_OK   "A vector of 3"
            jsr         kind_of
            EXPECT_A    PK_C8, "In a cell of 8"
            jsr         root_push
            lda         roots_lo                            ; (Its values: the short string ...
            sta         h_a
            lda         roots_hi
            sta         h_a + 1
            jsr         root_get
            ldy         #0
            jsr         vector_set
            lda         roots_lo + 1                        ;   the long one ...
            sta         h_a
            lda         roots_hi + 1
            sta         h_a + 1
            jsr         root_get
            ldy         #1
            jsr         vector_set
            LDR         h_a, FIX(42)                        ;   and 42)
            jsr         root_get
            ldy         #2
            jsr         vector_set
            jsr         root_get                            ; (The vector the one root: the strings only in it)
            jsr         root_drop
            jsr         root_drop
            jsr         root_drop
            jsr         root_push
            LDR         i, 2000                             ; (Garbage, to be collected over the strings' cells)
            jsr         churn
            jsr         gc
            LDR         i, 2000
            jsr         churn
            jsr         root_get
            ldy         #2
            jsr         vector_ref
            lda         h_v
            EXPECT_A    <FIX(42), "The vector's third value, after collections"
            jsr         root_get
            ldy         #0
            jsr         vector_ref
            jsr         deref
            ldy         #1
            lda         (h_p),y
            EXPECT_A    5, "Its first: the string of 5"
            ldy         #6
            lda         (h_p),y
            EXPECT_A    'o', "Its last byte"
            jsr         root_get
            ldy         #1
            jsr         vector_ref
            jsr         deref
            ldy         #101
            lda         (h_p),y
            EXPECT_A    'z', "Its second: the string of 100, its last byte"
            jsr         root_drop

; ---- A list of 5000 whose items are lists (n n): the mark stack (4096) overflows, and a rescan marks what the
; items it couldn't take reach
            jsr         gc
            LDR         i, 5000
            jsr         build_nested
            EXPECT_OK   "A list of 5000 lists (n n)"
            lda         gc_rescans
            sta         w
            jsr         gc
            lda         gc_rescans
            cmp         w
            bne         :+
            NOTOK       "Collected: the mark stack full, and the marked cells scanned again"
            bra         :++
:
            OK          "Collected: the mark stack full, and the marked cells scanned again"
:
            lda         mark_over
            EXPECT_A    0, "Nothing left to scan"
            LDR         i, 3000
            jsr         churn
            jsr         check_nested
            EXPECT_OK   "Every pair kept, after more garbage"
            jsr         root_drop

; ---- The heap filled: a list kept till there's no room; then let go, and room again
            jsr         gc
            LDR         h_v, NIL
            jsr         root_push
            stz         len
            stz         len + 1
@fill:
            LDR         h_a, FIX(1)
            jsr         root_get
            MOVR        h_d, h_v
            jsr         cons
            bcs         @full
            jsr         root_set
            inc         len
            bne         @fill
            inc         len + 1
            bra         @fill
@full:
            lda         len + 1                             ; (253 pages of 64 pairs: 16,192)
            cmp         #>16000
            bcs         :+
            NOTOK       "The heap full at 16,000 pairs or more"
            bra         :++
:
            OK          "The heap full at 16,000 pairs or more"
:
            jsr         root_drop
            LDR         h_a, NIL
            LDR         h_d, NIL
            jsr         cons
            EXPECT_OK   "Room again, the list let go"
            jsr         gc
            jsr         free_pages
            EXPECT_A    253, "Every page free again"

            DONE        "t_heap"

; ---- Lists

; A list of i values, (0 1 ... i-1), as a new root.  OUT: C = 1, no room
build:
            LDR         h_v, NIL
            jsr         root_push
@next:
            lda         i
            ora         i + 1
            beq         @done
            lda         i
            bne         :+
            dec         i + 1
:
            dec         i
            lda         i                                   ; (h_a = i, a fixnum)
            asl
            ora         #1
            sta         h_a
            lda         i + 1
            rol
            sta         h_a + 1
            jsr         root_get
            MOVR        h_d, h_v
            jsr         cons
            bcs         @fail
            jsr         root_set
            bra         @next
@done:
            clc
@fail:
            rts

; As build, but a pair not kept made before each of the list's
build_combed:
            LDR         h_v, NIL
            jsr         root_push
@next:
            lda         i
            ora         i + 1
            beq         @done
            lda         i
            bne         :+
            dec         i + 1
:
            dec         i
            LDR         h_a, NIL
            LDR         h_d, NIL
            jsr         cons
            bcs         @fail
            lda         i
            asl
            ora         #1
            sta         h_a
            lda         i + 1
            rol
            sta         h_a + 1
            jsr         root_get
            MOVR        h_d, h_v
            jsr         cons
            bcs         @fail
            jsr         root_set
            bra         @next
@done:
            clc
@fail:
            rts

; The last root's list: C = 0 if it's (0 1 ... n-1), its length in len
check:
            jsr         root_get
            stz         len
            stz         len + 1
@walk:
            lda         h_v
            ora         h_v + 1
            beq         @done
            MOVR        v, h_v
            jsr         car
            lda         h_v + 1                             ; (Its value: the fixnum >> 1)
            lsr
            sta         w + 1
            lda         h_v
            ror
            cmp         len                                 ; (The nth: n)
            bne         @bad
            lda         w + 1
            cmp         len + 1
            bne         @bad
            MOVR        h_v, v
            jsr         cdr
            inc         len
            bne         @walk
            inc         len + 1
            bra         @walk
@done:
            clc
            rts
@bad:
            sec
            rts

; i pairs made and not kept
churn:
            LDR         h_a, FIX(5)
            LDR         h_d, NIL
@next:
            jsr         cons
            bcs         @done
            lda         i
            bne         :+
            dec         i + 1
:
            dec         i
            lda         i
            ora         i + 1
            bne         @next
@done:
            rts

; A list of i items, each a list (n n), n from 0, as a new root.  OUT: C = 1, no room
build_nested:
            LDR         h_v, NIL
            jsr         root_push
@next:
            lda         i
            ora         i + 1
            beq         @done
            lda         i
            bne         :+
            dec         i + 1
:
            dec         i
            lda         i                                   ; (The item: (i i))
            asl
            ora         #1
            sta         h_a
            lda         i + 1
            rol
            sta         h_a + 1
            LDR         h_d, NIL
            jsr         cons
            bcs         @fail
            MOVR        h_d, h_v                            ; (Its tail: h_d, a root meanwhile)
            jsr         cons
            bcs         @fail
            MOVR        h_a, h_v                            ; (Onto the list: h_a, a root meanwhile)
            jsr         root_get
            MOVR        h_d, h_v
            jsr         cons
            bcs         @fail
            jsr         root_set
            bra         @next
@done:
            clc
@fail:
            rts

; The last root's list of lists (n n): C = 0 if each is the one it should be
check_nested:
            jsr         root_get
            stz         len
            stz         len + 1
@walk:
            lda         h_v
            ora         h_v + 1
            beq         @done
            MOVR        v, h_v
            jsr         car                                 ; (The item: a pair ...
            jsr         kind_of
            cmp         #PK_PAIR
            bne         @bad
            jsr         cdr                                 ;   whose cdr is a pair ...
            jsr         kind_of
            cmp         #PK_PAIR
            bne         @bad
            jsr         car                                 ;   whose car is n)
            lda         h_v + 1
            lsr
            sta         w + 1
            lda         h_v
            ror
            cmp         len
            bne         @bad
            lda         w + 1
            cmp         len + 1
            bne         @bad
            MOVR        h_v, v
            jsr         cdr
            inc         len
            bne         @walk
            inc         len + 1
            bra         @walk
@done:
            lda         len + 1
            cmp         #>5000
            bne         @bad
            lda         len
            cmp         #<5000
            bne         @bad
            clc
            rts
@bad:
            sec
            rts

.rodata
s_hello:    .byte       "hello"
; (100 bytes, the last a z)
s_long:     .byte       "0123456789012345678901234567890123456789012345678901234567890123456789"
            .byte       "abcdabcdefghijklmnopqrstuvwxyz"
.code

.include "../../modules/hylang/heap.inc"
