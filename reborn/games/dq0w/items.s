; ****************************************************************************
; items.s - (dq0w) the item store's records reached quickly (things.c's; the senses' too): item_at (), a record's
; bank in the window and the record, without cc65's multiplying (24 bytes a record); and items (), the next record
; after one that's in use and where it's asked for (in a thing, on a square ...: a bit of each, IN_*), by the x it's
; at (or by its flags), found by going down the banks in assembly (a look about, an inventory, went through every
; record in C a few times over: two seconds of an inventory).  hub.h's ITEM_BANK, ITEM_AT and items.h's struct item
; (things.c checks they're as here).
;
;   struct item* __fastcall__ item_at (unsigned i);                         item i's record, its bank in the window
;   unsigned __fastcall__ items (unsigned i, unsigned char in, unsigned x); the next after i (0: none), ip its record

            .export     _item_at, _items, _ip
            .import     _hub_map, incsp3
            .importzp   sp, ptr1

REC         = 24                            ; sizeof (struct item)
PER_BANK    = 341                           ; Records a bank (0x2000 / 24)
NITEMS      = 682                           ; (ITEM_BANKS of them)
BANK0       = 14                            ; ITEM_BANK (0): CAT_BANK + 1
LAST_BANK   = BANK0 + 1
END         = $8000 + PER_BANK * REC      ; (A bank's records' end)
IN_X        = $80                           ; items ()'s in: its x is x (else any, but IN_FLAGS)
IN_FLAGS    = $40                           ;   or its flags and x's low byte not 0 (I_LIT ...)

            .bss
_ip:        .res        2                   ; items ()'s record, its bank in the window
at:         .res        2                   ; (The record's number,
bank:       .res        1                   ;   its bank)
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

; Record at's bank in the window, ip the record (at the bank's own: $8000 + its place there times 24)
find:
            ldx         #BANK0
            lda         at
            sec
            sbc         #<PER_BANK
            tay
            lda         at + 1
            sbc         #>PER_BANK
            bcc         @first
            inx                             ; (The second bank's: at - 341)
            sty         _ip
            sta         _ip + 1
            jmp         @mul
@first:     lda         at
            sta         _ip
            lda         at + 1
            sta         _ip + 1
@mul:       stx         bank
            asl         _ip                 ; Its place times 8, kept in ptr1,
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
            lda         _ip                 ;   and the two: times 24, from $8000
            clc
            adc         ptr1
            sta         _ip
            lda         _ip + 1
            adc         ptr1 + 1
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
            inc         at                  ; From the one after i,
            bne         @in
            inc         at + 1
@in:        lda         at                  ;   if there is one
            cmp         #<NITEMS
            lda         at + 1
            sbc         #>NITEMS
            bcc         @find
            jmp         @none
@find:      jsr         find
            lda         _ip
            sta         ptr1
            lda         _ip + 1
            sta         ptr1 + 1
@look:      ldy         #0                  ; In use,
            lda         (ptr1),y
            beq         @next
            ldy         #2                  ;   where it's wanted,
            lda         (ptr1),y
            and         #7
            tax
            lda         place,x
            and         want
            beq         @next
            bit         want
            bmi         @byx
            bvc         @found
            ldy         #1                  ;   its flags,
            lda         (ptr1),y
            and         xv
            beq         @next
            bne         @found
@byx:       ldy         #4                  ;   or its x
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
@on:        lda         ptr1
            clc
            adc         #REC
            sta         ptr1
            bcc         @left
            inc         ptr1 + 1
@left:      lda         ptr1 + 1            ;   to the bank's end ($8000 + 341 * 24)
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
