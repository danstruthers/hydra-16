; ****************************************************************************
; items.s - (dq0w) the item store's records reached quickly (things.c's; the senses' too): item_at (), a record's
; bank in the window and the record, without cc65's multiplying (25 bytes a record); and items (), the next record
; after one that's in use and where it's asked for (in a thing, on a square ...: a bit of each, IN_*), by the x it's
; at (or by its flags), found by going down the banks in assembly (a look about, an inventory, went through every
; record in C a few times over: two seconds of an inventory).  hub.h's ITEM_BANK, ITEM_AT and items.h's struct item
; (their numbers the build's, from the headers: dq0hub_items.inc, ../dq0/hubgen.js).
;
;   struct item* __fastcall__ item_at (unsigned i);                         item i's record, its bank in the window
;   unsigned __fastcall__ items (unsigned i, unsigned char in, unsigned x); the next after i (0: none), ip its record
;   unsigned char __fastcall__ den_draw (unsigned char want);                 the world's alone (wbody.h's): below

            .export     _item_at, _items, _ip
            .import     _hub_map, incsp3
.ifndef ITEMS_ALL                           ; (The world's: things.c's, the records past it all free; a reader of
            .import     _ihigh              ;   its store, the senses, looks at every record, ITEMS_ALL)
.endif
            .importzp   sp, ptr1

            .include    "dq0hub_items.inc"          ; (REC sizeof (struct item), NITEMS, BANK0 ITEM_BANK (0), ITEM_BANKS,
                                                    ;   the fields I_*: the build's, ../dq0/hubgen.js)
PER_BANK    = $2000 / REC                   ; Records a bank
LAST_BANK   = BANK0 + ITEM_BANKS - 1
END         = $8000 + PER_BANK * REC      ; (A bank's records' end)
IN_X        = $80                           ; items ()'s in: its x is x (else any, but IN_FLAGS)
IN_FLAGS    = $40                           ;   or its flags and x's low byte not 0 (I_LIT ...)

            .bss
_ip:        .res        2                   ; items ()'s record, its bank in the window
at:         .res        2                   ; (The record's number,
bank:       .res        1                   ;   its bank)
one:        .res        2                   ; (find's place, times one)
want:       .res        1                   ; (items ()'s in,
xv:         .res        2                   ;   and x)

            .rodata
place:      .byte       1, 2, 4, 8, 16, 32, 64, 128     ; (A record's where as in's bit: IW_SQUARE 1, IW_ACTOR 2 ...)

            .code

; item_at (i): i in .A .X
_item_at:
            sta         at
            stx         at + 1
            jsr         find
            lda         _ip
            ldx         _ip + 1
            rts

; Record at's bank in the window, ip the record (at the bank's own: $8000 + its place there times 25)
find:
            ldx         #BANK0
            lda         at
            sec
            sbc         #<PER_BANK
            tay
            lda         at + 1
            sbc         #>PER_BANK
            bcc         @first
            inx                             ; (The second bank's: at - 327)
            sty         _ip
            sta         _ip + 1
            bra         @mul
@first:     lda         at
            sta         _ip
            lda         at + 1
            sta         _ip + 1
@mul:       stx         bank
            lda         _ip                 ; Its place kept,
            sta         one
            lda         _ip + 1
            sta         one + 1
            asl         _ip                 ;   times 8, kept in ptr1,
            rol         _ip + 1
            asl         _ip
            rol         _ip + 1
            asl         _ip
            rol         _ip + 1
            lda         _ip
            sta         ptr1
            lda         _ip + 1
            sta         ptr1 + 1
            asl         _ip                 ;   times 16,
            rol         _ip + 1
            lda         _ip                 ;   and the two: times 24,
            clc
            adc         ptr1
            sta         _ip
            lda         _ip + 1
            adc         ptr1 + 1
            sta         _ip + 1
            lda         _ip                 ;   and once more: times 25, from $8000
            clc
            adc         one
            sta         _ip
            lda         _ip + 1
            adc         one + 1
            ora         #$80
            sta         _ip + 1
            lda         bank
            jmp         _hub_map

; items (i, in, x): x in .A .X, in and i on the C stack
_items:
            sta         xv
            stx         xv + 1
            ldy         #0
            lda         (sp),y
            sta         want
            iny
            lda         (sp),y
            sta         at
            iny
            lda         (sp),y
            sta         at + 1
            jsr         incsp3
items_from  =           *                   ; (den_draw's: from the one after at, want's, x none)
            inc         at                  ; From the one after i,
            bne         @in
            inc         at + 1
@in:        lda         at                  ;   if there is one (the records in use all before ihigh,
.ifdef ITEMS_ALL                            ;   things.c's: 2026-10-27, an inventory's pass of them the
            cmp         #<NITEMS            ;   shorter; the senses', every one)
            lda         at + 1
            sbc         #>NITEMS
.else
            cmp         _ihigh
            lda         at + 1
            sbc         _ihigh + 1
.endif
            bcc         @find
            jmp         @none
@find:      jsr         find
            lda         _ip
            sta         ptr1
            lda         _ip + 1
            sta         ptr1 + 1
@look:      ldy         #I_KIND             ; In use,
            lda         (ptr1),y
            beq         @next
            ldy         #I_WHERE            ;   where it's wanted,
            lda         (ptr1),y
            and         #7
            tax
            lda         place,x
            and         want
            beq         @next
            bit         want
            bmi         @byx
            bvc         @found
            ldy         #I_FLAGS            ;   its flags,
            lda         (ptr1),y
            and         xv
            beq         @next
            bne         @found
@byx:       ldy         #I_X                ;   or its x
            lda         (ptr1),y
            cmp         xv
            bne         @next
            iny
            lda         (ptr1),y
            cmp         xv + 1
            beq         @found
@next:      inc         at                  ; The next record,
            bne         @on
            inc         at + 1
@on:
.ifndef ITEMS_ALL
            lda         at                  ;   none past ihigh
            cmp         _ihigh
            lda         at + 1
            sbc         _ihigh + 1
            bcs         @none
.endif
@more:      lda         ptr1
            clc
            adc         #REC
            sta         ptr1
            bcc         @left
            inc         ptr1 + 1
@left:      lda         ptr1 + 1            ;   to the bank's end ($8000 + 327 * 25)
            cmp         #>END
            bne         @look
            lda         ptr1
            cmp         #<END
            bcc         @look
            lda         bank                ;   or the next bank's first
            cmp         #LAST_BANK
            bcs         @none
            inc         bank
            lda         bank
            jsr         _hub_map
            lda         #$00
            sta         ptr1
            lda         #$80
            sta         ptr1 + 1
            jmp         @look
@found:     lda         ptr1
            sta         _ip
            lda         ptr1 + 1
            sta         _ip + 1
            lda         at
            ldx         at + 1
            rts
@none:      lda         #0
            tax
            rts

.ifndef ITEMS_ALL
; den_draw (want): body bd's weapon drawn as it strikes, or a thing of its own of kind want taken in hand (wbody.h's), by
; part den_things's den_draws, asked only of one with a thing of its own (a record IW_DEN, its owner and hide its did): none, 0,
; as den_draws's own first test has it (kit_next's), so a beast's every blow doesn't copy part den_things in over part fight_blows and
; part fight_blows back after it, some 78,000 cycles of each round a denizen strikes in (speed11's, 2026-11-22: dq0fightcmd's
; wait typed as a rat fought, 760,174).  The window as den_draws's test left it: an item bank, or as it was (no did)
            .export     _den_draw
            .import     _den_draws, _bd
            .bss
dwant:      .res        1
            .code
_den_draw:
            sta         dwant
            lda         _bd + A_DID
            ora         _bd + A_DID + 1
            beq         @none
            lda         #1 << IW_DEN
            sta         want
            stz         at
            stz         at + 1
@again:     jsr         items_from          ; (Its next IW_DEN record: ptr1 and ip at it)
            cpx         #0
            bne         @mine
            cmp         #0
            beq         @none
@mine:      ldy         #I_OWNER            ; (Its own: its did)
            lda         (ptr1),y
            cmp         _bd + A_DID
            bne         @again
            ldy         #I_HIDE
            lda         (ptr1),y
            cmp         _bd + A_DID + 1
            bne         @again
            lda         dwant
            jmp         _den_draws
@none:      lda         #0
            tax
            rts
.endif
