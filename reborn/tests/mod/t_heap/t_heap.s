; ****************************************************************************
; t_heap - hylang's runtime (modules/hylang/heap.inc: phase 1), run as init: values and fixnums, cells, symbols and
; atoms, strings; the collector (a list kept while garbage is taken back, a structure deeper than the mark stack,
; blobs dropped and the rest moved down); the heap growing as it fills, a million cells made and dropped with none
; lost, and its end (E_NOMEM).  The times, between marks: 1000 conses made ("<cons" "cons>"), and a collection with
; 1000 cells live ("<gc1k" "gc1k>") and with 9000 ("<gc9k" "gc9k>").

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"
.include "../../modules/hylang/hylang.inc"

            HYX2_PROGRAM "t_heap", main

.include "../../modules/hylang/heap.inc"

.zeropage
ti:         .res        2                                   ; A count
tj:         .res        2                                   ; Another
tsum:       .res        2                                   ; A list's numbers added up (16 bits)
tlen:       .res        2                                   ; A list's length
tw:         .res        2                                   ; A value walked

.bss
base:       .res        2                                   ; The free room, as it was
tops:       .res        2                                   ; The first blob bank's top, before a collection
text:       .res        16                                  ; A string's bytes, made

.code

main:
            stz         T_FAILS
            jsr         heap_init
            EXPECT_OK   "heap_init: two banks of cells, one of blobs"
            lda         free_pages
            EXPECT_A    26, "26 free pages (32, less the immediates' 6)"

; ---- Values
            lda         #<NIL
            ldx         #>NIL
            jsr         kind_of
            EXPECT_A    PK_IMM, "NIL: an immediate"
            lda         #<(CHAR0 + 2 * 'A')
            ldx         #>(CHAR0 + 2 * 'A')
            jsr         kind_of
            EXPECT_A    PK_IMM, "a character: an immediate"
            lda         #<FIX(-5)
            ldx         #>FIX(-5)
            jsr         kind_of
            EXPECT_A    PK_FIXNUM, "a fixnum"
            lda         #<-1234
            ldx         #>-1234
            jsr         fix_make
            EXPECT_OK   "fix_make -1234"
            lda         hv
            EXPECT_A    <FIX(-1234), "its value"
            jsr         fix_value
            stx         tw
            EXPECT_A    <-1234, "fix_value: its number back (low)"
            lda         tw
            EXPECT_A    >-1234, "(high)"
            lda         #<-16384
            ldx         #>-16384
            jsr         fix_make
            EXPECT_OK   "fix_make -16384, the least"
            lda         #<16384
            ldx         #>16384
            jsr         fix_make
            bcs         :+
            NOTOK       "fix_make 16384: out of range"
            bra         :++
:
            OK          "fix_make 16384: out of range"
:

; ---- Cells
            lda         #PK_QCONS
            jsr         cell_alloc
            EXPECT_OK   "cell_alloc: a cons"
            lda         hv
            ldx         hv + 1
            jsr         kind_of
            EXPECT_A    PK_QCONS, "its kind"
            ldy         #1
            lda         #<FIX(7)
            ldx         #>FIX(7)
            jsr         cell_set
            lda         hv
            ldx         hv + 1
            ldy         #1
            jsr         cell_get
            EXPECT_A    <FIX(7), "cell_set, cell_get: its cdr"
            lda         hv
            ldx         hv + 1
            ldy         #0
            jsr         cell_get
            EXPECT_A    <NIL, "its car NIL, as it was made"

; ---- Symbols and atoms, strings
            LDR         hq, s_hello
            lda         #5
            sta         hn
            stz         hn + 1
            lda         #PK_SYMBOL
            jsr         intern
            EXPECT_OK   "intern: the symbol hello"
            MOVR        VR0, hv
            lda         #PK_SYMBOL
            jsr         intern
            jsr         same_as_vr0
            EXPECT_A    1, "the same name: the same symbol"
            lda         #PK_ATOM
            jsr         intern
            jsr         same_as_vr0
            EXPECT_A    0, "the atom :hello: another"
            LDR         hq, s_hellp
            lda         #PK_SYMBOL
            jsr         intern
            jsr         same_as_vr0
            EXPECT_A    0, "hellp: another"
            lda         VR0
            ldx         VR0 + 1
            ldy         #1
            jsr         cell_get
            EXPECT_A    <UNBOUND, "hello's value: UNBOUND"
            lda         VR0                                 ; (Its name, in its blob)
            ldx         VR0 + 1
            ldy         #0
            jsr         cell_get
            jsr         blob_data
            lda         hn
            EXPECT_A    5, "its name: 5 bytes ..."
            LDR         hq, s_hello
            jsr         same_bytes
            EXPECT_A    0, "... hello"
            LDR         hq, s_text
            lda         #11
            sta         hn
            stz         hn + 1
            jsr         string_make
            EXPECT_OK   "string_make"
            lda         hv
            ldx         hv + 1
            jsr         kind_of
            EXPECT_A    PK_STRING, "a string"
            lda         hv
            ldx         hv + 1
            ldy         #0
            jsr         cell_get
            EXPECT_A    11, "its length"
            lda         hv
            ldx         hv + 1
            ldy         #1
            jsr         cell_get
            jsr         blob_data
            LDR         hq, s_text
            jsr         same_bytes
            EXPECT_A    0, "its bytes"

; ---- 1000 conses made (timed), a list kept in VR1
            stz         VR1
            stz         VR1 + 1
            MARK        "<cons"
            jsr         list_1000
            MARK        "cons>"
            EXPECT_OK   "1000 conses"
            jsr         check_1000
            EXPECT_A    0, "a list of 1000: its length and its numbers"

; ---- A collection with 1000 cells live (timed): the list kept, the rest free
            MARK        "<gc1k"
            jsr         gc_collect
            MARK        "gc1k>"
            jsr         check_1000
            EXPECT_A    0, "after a collection: the list as it was"
            jsr         heap_free
            MOVR        base, hn

; ---- Garbage: 5000 conses dropped, then taken back
            LDR         ti, 5000
@garbage:
            stz         VR6
            stz         VR6 + 1
            stz         VR7
            stz         VR7 + 1
            jsr         tcons
            bcs         @garbaged
            jsr         dec_ti
            bne         @garbage
@garbaged:
            EXPECT_OK   "5000 conses made and dropped"
            jsr         clear_scratch
            jsr         gc_collect
            jsr         check_1000
            EXPECT_A    0, "the list kept"
            jsr         heap_free
            jsr         hn_is_base
            EXPECT_A    0, "the garbage taken back: the free room as it was"

; ---- A structure deeper than the mark stack (255): 1000 conses, each the car of the next
            stz         VR2
            stz         VR2 + 1
            LDR         ti, 1000
@deep:
            MOVR        VR6, VR2
            stz         VR7
            stz         VR7 + 1
            jsr         tcons
            bcs         @deeped
            MOVR        VR2, hv
            jsr         dec_ti
            bne         @deep
@deeped:
            EXPECT_OK   "1000 conses, nested in their cars"
            jsr         gc_collect
            MOVR        tw, VR2                             ; (Its depth: cars to NIL)
            stz         tlen
            stz         tlen + 1
@depth:
            lda         tw
            ora         tw + 1
            beq         @deepest
            inc         tlen
            bne         :+
            inc         tlen + 1
:
            lda         tw
            ldx         tw + 1
            ldy         #0
            jsr         cell_get
            sta         tw
            stx         tw + 1
            bra         @depth
@deepest:
            lda         tlen + 1
            EXPECT_A    >1000, "after a collection (the mark stack full): 1000 deep (high)"
            lda         tlen
            EXPECT_A    <1000, "(low)"
            stz         VR2
            stz         VR2 + 1
            jsr         clear_scratch
            jsr         gc_collect
            jsr         heap_free
            jsr         hn_is_base
            EXPECT_A    0, "dropped: the free room as it was"

; ---- Blobs: 100 strings, every other kept; a collection drops the rest and moves the kept down
            stz         VR3
            stz         VR3 + 1
            stz         ti
@string:
            jsr         text_of_ti
            LDR         hq, text
            lda         #8
            sta         hn
            stz         hn + 1
            jsr         string_make
            bcs         @strung
            lda         ti
            lsr
            bcs         @odd
            MOVR        VR6, hv                             ; (An even one, kept)
            MOVR        VR7, VR3
            jsr         tcons
            bcs         @strung
            MOVR        VR3, hv
@odd:
            inc         ti
            lda         ti
            cmp         #100
            bcc         @string
            clc
@strung:
            EXPECT_OK   "100 strings, 50 kept"
            lda         blob_tlo
            sta         tops
            lda         blob_thi
            sta         tops + 1
            jsr         gc_collect
            lda         tops + 1                            ; (The first bank's top: lower)
            cmp         blob_thi
            bne         :+
            lda         tops
            cmp         blob_tlo
:
            beq         :+
            bcs         :++
:
            NOTOK       "a collection: the blobs moved down"
            bra         :++
:
            OK          "a collection: the blobs moved down"
:
            lda         #98                                 ; (The kept: 98, 96 ... 0, each its bytes)
            sta         ti
            MOVR        tw, VR3
            stz         tj
@kept:
            lda         tw
            ora         tw + 1
            beq         @keptall
            jsr         text_of_ti
            lda         tw
            ldx         tw + 1
            ldy         #0
            jsr         cell_get                            ; (The string ...
            ldy         #1
            jsr         cell_get                            ;   its blob)
            jsr         blob_data
            LDR         hq, text
            jsr         same_bytes
            ora         tj
            sta         tj
            lda         tw
            ldx         tw + 1
            ldy         #1
            jsr         cell_get
            sta         tw
            stx         tw + 1
            dec         ti
            dec         ti
            bra         @kept
@keptall:
            lda         tj
            EXPECT_A    0, "the 50 kept, each its bytes"
            stz         hv                                  ; (A blob bank full to its last byte, every blob in it
            stz         hv + 1                              ;   live: hv's last string let go and the dead dropped
            jsr         gc_collect                          ;   first, then a string that fills the first, kept)
            sec
            lda         #0
            sbc         blob_tlo
            sta         hn
            lda         #$20
            sbc         blob_thi
            sta         hn + 1
            lda         hn
            sec
            sbc         #4
            sta         hn
            bcs         :+
            dec         hn + 1
:
            LDR         hq, text                            ; (Its bytes: any)
            jsr         string_make
            EXPECT_OK   "a string that fills the first blob bank"
            MOVR        VR6, hv
            lda         blob_thi
            EXPECT_A    $20, "the first blob bank full"
            jsr         gc_collect
            lda         blob_thi                            ; (Still full: its top $2000, not 0)
            cmp         #$20
            bne         :+
            lda         blob_tlo
            bne         :+
            OK          "a collection: the full blob bank kept full"
            bra         :++
:
            NOTOK       "a collection: the full blob bank kept full"
:
            stz         VR6
            stz         VR6 + 1
            stz         VR3
            stz         VR3 + 1

; ---- 9000 cells live (VR1's 1000, VR4's 8000): the heap grows; a collection (timed)
            stz         VR4
            stz         VR4 + 1
            LDR         ti, 8000
@more:
            lda         ti
            ldx         ti + 1
            jsr         fix_make
            MOVR        VR6, hv
            MOVR        VR7, VR4
            jsr         tcons
            bcs         @mored
            MOVR        VR4, hv
            jsr         dec_ti
            bne         @more
@mored:
            EXPECT_OK   "8000 conses more"
            lda         cell_banks
            cmp         #5
            bcs         :+
            NOTOK       "the heap grown: 5 banks of cells or more"
            bra         :++
:
            OK          "the heap grown: 5 banks of cells or more"
:
            MARK        "<gc9k"
            jsr         gc_collect
            MARK        "gc9k>"
            jsr         check_1000
            EXPECT_A    0, "after a collection: the list of 1000 ..."
            MOVR        tw, VR4
            jsr         length_tw
            lda         tlen + 1
            EXPECT_A    >8000, "... and the list of 8000 (high)"
            lda         tlen
            EXPECT_A    <8000, "(low)"

; ---- A million cells made and dropped: none lost
            stz         VR1
            stz         VR1 + 1
            stz         VR4
            stz         VR4 + 1
            jsr         clear_scratch
            jsr         gc_collect
            jsr         heap_free
            MOVR        base, hn
            LDR         tj, 1000
@thousand:
            LDR         ti, 1000
            stz         VR5
            stz         VR5 + 1
@one:
            MOVR        VR6, ti
            MOVR        VR7, VR5
            jsr         tcons
            bcs         @million
            MOVR        VR5, hv
            lda         ti                                  ; (Every 100th: the chain dropped)
            cmp         #100
            bne         :+
            stz         VR5
            stz         VR5 + 1
:
            jsr         dec_ti
            bne         @one
            lda         tj
            bne         :+
            dec         tj + 1
:
            dec         tj
            lda         tj
            ora         tj + 1
            bne         @thousand
            clc
@million:
            EXPECT_OK   "a million conses made, 100 at most kept"
            stz         VR5
            stz         VR5 + 1
            jsr         clear_scratch
            jsr         gc_collect
            jsr         heap_free
            jsr         hn_is_base
            EXPECT_A    0, "none lost: the free room as it was"

; ---- The end of room: conses kept till there's no room (16 banks of cells), then E_NOMEM
            stz         VR1
            stz         VR1 + 1
@fill:
            stz         VR6
            stz         VR6 + 1
            MOVR        VR7, VR1
            jsr         tcons
            bcs         @full
            MOVR        VR1, hv
            bra         @fill
@full:
            EXPECT_ERR  E_NOMEM, "conses kept till E_NOMEM"
            lda         cell_banks
            EXPECT_A    16, "16 banks of cells taken"
            stz         VR1
            stz         VR1 + 1
            jsr         clear_scratch
            jsr         gc_collect
            jsr         heap_free
            lda         hn + 1                              ; (Room for 31,000 units again: $7918)
            cmp         #$79
            bcs         :+
            NOTOK       "dropped: the room free again"
            bra         :++
:
            OK          "dropped: the room free again"
:
            DONE        "t_heap"

; ---- Helpers

; A cons of VR6 and VR7, made: hv.  OUT: C = 1 if there's no room
tcons:
            lda         #PK_QCONS
            jsr         cell_alloc
            bcs         @rts
            lda         VR6                                 ; (cell_alloc leaves hp at it, its bank in the window)
            sta         (hp)
            ldy         #1
            lda         VR6 + 1
            sta         (hp),y
            iny
            lda         VR7
            sta         (hp),y
            iny
            lda         VR7 + 1
            sta         (hp),y
            clc
@rts:
            rts

; hv, VR6 and VR7 (roots the helpers use) NIL, so what they held is garbage
clear_scratch:
            stz         hv
            stz         hv + 1
            stz         VR6
            stz         VR6 + 1
            stz         VR7
            stz         VR7 + 1
            rts

; ti counted down.  OUT: Z = 1 when it's 0
dec_ti:
            lda         ti
            bne         :+
            dec         ti + 1
:
            dec         ti
            lda         ti
            ora         ti + 1
            rts

; VR1 = a list of the fixnums 1-1000 (consed from 1000 down).  OUT: C = 1 if there's no room
list_1000:
            LDR         ti, 1000
@cons:
            lda         ti
            ldx         ti + 1
            jsr         fix_make
            MOVR        VR6, hv
            MOVR        VR7, VR1
            jsr         tcons
            bcs         @rts
            MOVR        VR1, hv
            jsr         dec_ti
            bne         @cons
            clc
@rts:
            rts

; VR1 checked: 1000 long, its numbers 1-1000 (their sum 500500, $A314 in 16 bits).  OUT: .A = 0 if it's so
check_1000:
            MOVR        tw, VR1
            stz         tsum
            stz         tsum + 1
            stz         tlen
            stz         tlen + 1
@item:
            lda         tw
            ora         tw + 1
            beq         @end
            lda         tw
            ldx         tw + 1
            ldy         #0
            jsr         cell_get
            sta         hv
            stx         hv + 1
            jsr         fix_value
            clc
            adc         tsum
            sta         tsum
            txa
            adc         tsum + 1
            sta         tsum + 1
            inc         tlen
            bne         :+
            inc         tlen + 1
:
            lda         tw
            ldx         tw + 1
            ldy         #1
            jsr         cell_get
            sta         tw
            stx         tw + 1
            bra         @item
@end:
            lda         tlen
            eor         #<1000
            sta         tw
            lda         tlen + 1
            eor         #>1000
            ora         tw
            sta         tw
            lda         tsum
            eor         #<500500
            ora         tw
            sta         tw
            lda         tsum + 1
            eor         #>(500500 & $FFFF)
            ora         tw
            rts

; tlen = the length of the list tw
length_tw:
            stz         tlen
            stz         tlen + 1
@item:
            lda         tw
            ora         tw + 1
            beq         @end
            inc         tlen
            bne         :+
            inc         tlen + 1
:
            lda         tw
            ldx         tw + 1
            ldy         #1
            jsr         cell_get
            sta         tw
            stx         tw + 1
            bra         @item
@end:
            rts

; OUT: .A = 0 if hn is base (the free room as it was)
hn_is_base:
            lda         hn
            eor         base
            sta         tw
            lda         hn + 1
            eor         base + 1
            ora         tw
            rts

; OUT: .A = 1 if hv is VR0, else 0
same_as_vr0:
            lda         hv
            cmp         VR0
            bne         @no
            lda         hv + 1
            cmp         VR0 + 1
            bne         @no
            lda         #1
            rts
@no:
            lda         #0
            rts

; OUT: .A = 0 if the hn bytes at hp (the window) are those at hq
same_bytes:
            ldy         #0
@byte:
            cpy         hn
            beq         @same
            lda         (hp),y
            cmp         (hq),y
            bne         @differ
            iny
            bra         @byte
@same:
            lda         #0
            rts
@differ:
            lda         #1
            rts

; text = "string" and ti's two digits (8 bytes)
text_of_ti:
            ldx         #5
@copy:
            lda         s_string,x
            sta         text,x
            dex
            bpl         @copy
            lda         ti
            ldx         #'0' - 1
            sec
@tens:
            inx
            sbc         #10
            bcs         @tens
            adc         #'0' + 10
            stx         text + 6
            sta         text + 7
            rts

.rodata
s_hello:    .byte       "hello"
s_hellp:    .byte       "hellp"
s_text:     .byte       "hello world"
s_string:   .byte       "string"
