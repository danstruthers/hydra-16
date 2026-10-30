; ****************************************************************************
; tlist.s - (dq0w) the things' words put, and the list's next thing found, in assembly (things.c's put and next_in:
; an inventory's every word and every thing, 2026-10-30; the same words, quicker).
;
;   void __fastcall__ put (const char* s);      s after the words so far: into the UI bank's things_text at tp, as far
;                                               as tpe (put_bank's: text_new sets both); or, tq set (a main RAM text,
;                                               as far as tqe), there; its 0 after it.  The hub's header in the window
;                                               after the UI bank's (as the C one left it), tq's: as it was
;   unsigned next_got (void);                   (part 11's: next_in's, the list gathered, its bank in the window) the
;                                               next record after nx_n at nx_w, nx_x in GOT_AT's list (from gcur, the
;                                               one it gave last, if that's nx_n; else from the list's start): its
;                                               number, gcur its place; 0 if none
; ****************************************************************************

            .setcpu     "65C02"
            .export     _put, _tp, _tq, _tqe, _tpe, _put_bank
            .export     _next_got, _nx_n, _nx_x, _nx_w, _gcur, _ngot
            .import     _hub_map
            .importzp   ptr1, ptr2, ptr3, tmp1

GOT_AT      = $9B00                 ; (things.c's: the list, in the kinds' bank; struct got, 6 bytes: n, x, where, kind)

            .bss
_tp:        .res        2           ; The UI bank's text's end so far,
_tpe:       .res        2           ;   and how far it may go
_put_bank:  .res        1           ;   (its bank)
_tq:        .res        2           ; Or a main RAM text's (0: none),
_tqe:       .res        2           ;   and its end
_nx_n:      .res        2           ; next_got's arguments
_nx_x:      .res        2
_nx_w:      .res        1
_gcur:      .res        1           ; (The entry it gave last)
_ngot:      .res        1           ; (The list's length)

            .code

_put:       pha                     ; (s, while the window's moved)
            phx
            lda         _tq + 1
            bne         @main
            lda         _put_bank
            jsr         _hub_map
            lda         _tp
            sta         ptr2
            lda         _tp + 1
            sta         ptr2 + 1
            lda         _tpe
            sta         ptr3
            lda         _tpe + 1
            sta         ptr3 + 1
            plx
            pla
            jsr         copy
            lda         ptr2
            sta         _tp
            lda         ptr2 + 1
            sta         _tp + 1
            lda         #0
            jmp         _hub_map

@main:      lda         _tq
            sta         ptr2
            lda         _tq + 1
            sta         ptr2 + 1
            lda         _tqe
            sta         ptr3
            lda         _tqe + 1
            sta         ptr3 + 1
            plx
            pla
            jsr         copy
            lda         ptr2
            sta         _tq
            lda         ptr2 + 1
            sta         _tq + 1
            rts

; .A, .X (s) to ptr2, as far as ptr3 (not past it), to its 0; the 0 at ptr2 after, ptr2 at it
copy:       sta         ptr1
            stx         ptr1 + 1
            sec                         ; tmp1 = the room (ptr3 - ptr2, 255 at most)
            lda         ptr3
            sbc         ptr2
            tax
            lda         ptr3 + 1
            sbc         ptr2 + 1
            bcc         @none           ; (Past it already)
            beq         :+
            ldx         #$FF
:           stx         tmp1
            ldy         #0
@byte:      cpy         tmp1
            beq         @end
            lda         (ptr1),y
            beq         @end
            sta         (ptr2),y
            iny
            bra         @byte
@end:       lda         #0
            sta         (ptr2),y
            tya
            clc
            adc         ptr2
            sta         ptr2
            bcc         :+
            inc         ptr2 + 1
:           rts
@none:      ldy         #0
            bra         @end

            .segment    "SW11CODE"

_next_got:  ldx         #0              ; .X = the entry to look from
            lda         _nx_n
            ora         _nx_n + 1
            beq         @from
            ldx         _gcur           ; (After gcur's, if it's nx_n's)
            cpx         _ngot
            bcs         @first
            jsr         at_x
            ldy         #0
            lda         (ptr1),y
            cmp         _nx_n
            bne         @first
            iny
            lda         (ptr1),y
            cmp         _nx_n + 1
            bne         @first
            inx
            bra         @from
@first:     ldx         #0
@from:      jsr         at_x            ; ptr1 = entry .X's
@look:      cpx         _ngot
            bcs         @none
            ldy         #4              ; where?
            lda         (ptr1),y
            cmp         _nx_w
            bne         @next
            ldy         #2              ; x?
            lda         (ptr1),y
            cmp         _nx_x
            bne         @next
            iny
            lda         (ptr1),y
            cmp         _nx_x + 1
            bne         @next
            ldy         #1              ; n > nx_n?
            lda         (ptr1),y
            cmp         _nx_n + 1
            bcc         @next
            bne         @found
            dey
            lda         (ptr1),y
            cmp         _nx_n
            beq         @next
            bcs         @found
@next:      inx
            lda         ptr1
            clc
            adc         #6
            sta         ptr1
            bcc         @look
            inc         ptr1 + 1
            bra         @look
@found:     stx         _gcur
            ldy         #0
            lda         (ptr1),y
            pha
            iny
            lda         (ptr1),y
            tax
            pla
            rts
@none:      lda         #0
            tax
            rts

; ptr1 = GOT_AT + .X * 6.  Keeps .X
at_x:       txa
            sta         tmp1
            asl         a               ; (.X * 6 = .X * 4 + .X * 2: 16 bits, .X up to 200)
            sta         ptr1
            lda         #0
            rol         a
            sta         ptr1 + 1        ; ptr1 = .X * 2
            lda         ptr1
            asl         a
            sta         ptr3
            lda         ptr1 + 1
            rol         a
            sta         ptr3 + 1        ; ptr3 = .X * 4
            lda         ptr1
            clc
            adc         ptr3
            sta         ptr1
            lda         ptr1 + 1
            adc         ptr3 + 1
            adc         #>GOT_AT
            sta         ptr1 + 1
            rts
