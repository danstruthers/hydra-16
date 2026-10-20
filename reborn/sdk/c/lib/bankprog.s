; ****************************************************************************
; bankprog.s - bank programs (bankprog.h): children, o65 modules linked at $8000 (sdk/c/child.cfg, build.js's
; children), loaded into a RAM bank of the task's own and called like any function, on the caller's C stack.
;
; Loading a child (its file, NAME.bp, as ld65 writes it: o65, simple, its text, data and BSS one after another from
; $8000, its zero page from 0): its header held to that, its options passed over, its text and data read into its bank
; and its BSS cleared; its imports' names put after its BSS, each found in the caller's exports (its NAME.exp: "BX",
; 1, then each export its name's length, its name and its address, a 0 after the last), and its relocations applied:
; an import's its address, its zero page's the caller's (bank_setup's), the rest none (it's run where it was linked).
; Then its exports (its entries) kept after its BSS, over the names: each its name, a 0, its address; a 0 after the
; last.  A child's slot (BANK_MAX of them) keeps its file's name, its bank, its load's number (a handle to it holds
; that: a child given up and loaded again has another) and when it was last called; a bank's wanted and none's free
; (the slots all used, or the kernel's banks all taken), the one called least lately is given up for it.
;
; bank_jsr, the call: the handle checked (its child still loaded, the same load; if not, loaded again), its bank put
; in the window (the bank register, $00; the shared banks' U left as it is), the entry called with .A/.X and the C
; stack as they were, the bank register put back with .A/.X as the entry left them.  No child may call one: it's in
; the window itself.
; ****************************************************************************

            .setcpu     "65C02"
            .macpack    longbranch
            .export     _bank_setup, _bank_fn_, _bank_jsr, _bank_flush
            .export     _bank_err, _bank_missing, _bank_go
            .import     popa, popax

            .include    "zeropage.inc"
            .include    "hydra.inc"

NSLOT       = 6                     ; bankprog.h's BANK_MAX, BANK_FNS
NFN         = 12
FN_CHILD    = 0                     ; struct bank_fn
FN_ENTRY    = 2
FN_SLOT     = 4
FN_GEN      = 5
FN_AT       = 6
FN_SIZE     = 8
BANKREG     = $00                   ; The bank register: the window's bank
WIN         = $8000                 ; The window, and its end
WTOP        = $A000
RBUF        = 255                   ; A file's bytes read at a time (a READ costs some 24K cycles from a card,
                                    ;   whatever its size: the more at once the better, to a byte's count)
NAMEMAX     = 31                    ; An export's name, at most

BE_SETUP    = 1                     ; bankprog.h's BE_*
BE_FILE     = 2
BE_FORMAT   = 3
BE_ZP       = 4
BE_EXPORTS  = 5
BE_IMPORT   = 6
BE_BANK     = 7
BE_ENTRY    = 8

            .bss
_bank_err:  .res        1
_bank_missing: .res     16
_bank_go:   .res        2
expath:     .res        2           ; The caller's exports' file, and its children's zero page
zpbase:     .res        1
zpn:        .res        1
sused:      .res        NSLOT       ; Each slot: in use, its file's name, its bank, its load, when it was last
snl:        .res        NSLOT       ;   called, its exports' table (in its bank)
snh:        .res        NSLOT
sbank:      .res        NSLOT
sgen:       .res        NSLOT
stl:        .res        NSLOT
sth:        .res        NSLOT
sxl:        .res        NSLOT
sxh:        .res        NSLOT
fns:        .res        NFN * FN_SIZE   ; The handles
nfn:        .res        1
clock:      .res        2           ; The calls counted (when a slot was last called)
gencnt:     .res        1           ; The loads counted
slot:       .res        1           ; The slot loaded, or bound to
oldbank:    .res        1           ; The window's bank before
sava:       .res        1           ; (bank_jsr's .A, .X, and the entry)
savx:       .res        1
jvec:       .res        2
fd:         .res        1           ; The child's file, read through rbuf
rbuf:       .res        RBUF
rpos:       .res        1
rlen:       .res        1
efd:        .res        1           ; The exports' file, through ebuf
ebuf:       .res        RBUF
epos:       .res        1
elen:       .res        1
hdr:        .res        26          ; The child's o65 header
ename:      .res        NAMEMAX + 1 ; An export's name read, its length
enlen:      .res        1
nimp:       .res        1           ; Its imports: how many, their names, their addresses (in its bank)
impn:       .res        2
impa:       .res        2
iend:       .res        2           ; The image's end (its BSS's), where its exports go
cnt:        .res        2
rbase:      .res        2           ; A relocation's place, its value, its type
rval:       .res        2
rtype:      .res        1

            .code

; ---- int __fastcall__ bank_setup (const char* exports, unsigned char zp, unsigned char n)
_bank_setup:
            sta         zpn
            jsr         popa
            sta         zpbase
            jsr         popax
            sta         expath
            stx         expath + 1
            lda         #0
            tax
            rts

; ---- struct bank_fn* __fastcall__ bank_fn_ (const char* child, const char* entry): a handle (the one already made
; for the same names, or a new one), its child loaded and its entry found; or NULL
_bank_fn_:
            sta         ptr2                ; ptr2 the entry's name, ptr1 the child's
            stx         ptr2 + 1
            jsr         popax
            sta         ptr1
            stx         ptr1 + 1
            stz         _bank_err
            lda         #<fns               ; One with these names already?
            sta         ptr3
            lda         #>fns
            sta         ptr3 + 1
            ldx         nfn
@find:      beq         @new
            ldy         #FN_CHILD
            lda         (ptr3),y
            cmp         ptr1
            bne         @next
            iny
            lda         (ptr3),y
            cmp         ptr1 + 1
            bne         @next
            iny
            lda         (ptr3),y
            cmp         ptr2
            bne         @next
            iny
            lda         (ptr3),y
            cmp         ptr2 + 1
            beq         @bind
@next:      jsr         nextfn
            dex
            bra         @find
@new:       lda         nfn                 ; A new one (ptr3 at the next free)
            cmp         #NFN
            bcs         @full
            lda         expath + 1
            beq         @full
            inc         nfn
            ldy         #FN_CHILD
            lda         ptr1
            sta         (ptr3),y
            iny
            lda         ptr1 + 1
            sta         (ptr3),y
            iny
            lda         ptr2
            sta         (ptr3),y
            iny
            lda         ptr2 + 1
            sta         (ptr3),y
            iny
            lda         #$FF                ; (Its slot none yet)
            sta         (ptr3),y
            jsr         bind
            bcc         @good
            dec         nfn                 ; (Its child not loaded: the new handle given back)
            bra         @none
@bind:      jsr         bind
            bcs         @none
@good:      lda         ptr3
            ldx         ptr3 + 1
            rts
@full:      lda         #BE_SETUP
            sta         _bank_err
@none:      lda         #0
            tax
            rts

; ptr3 to the next handle
nextfn:
            clc
            lda         ptr3
            adc         #FN_SIZE
            sta         ptr3
            bcc         :+
            inc         ptr3 + 1
:           rts

; ---- bind: handle ptr3 made good: its child loaded (if it isn't), its slot and load kept in it, its entry's
; address.  C = 1 if it can't be (bank_err)
bind:
            ldy         #FN_CHILD           ; ptr1 the child's name
            lda         (ptr3),y
            sta         ptr1
            iny
            lda         (ptr3),y
            sta         ptr1 + 1
            jsr         findslot
            bcc         @have
            jsr         load
            bcc         @have
            rts
@have:      stx         slot
            lda         BANKREG             ; Its entry, in its exports' table in its bank
            sta         oldbank
            lda         sbank,x
            sta         BANKREG
            lda         sxl,x
            sta         ptr4
            lda         sxh,x
            sta         ptr4 + 1
            ldy         #FN_ENTRY
            lda         (ptr3),y
            sta         ptr2
            iny
            lda         (ptr3),y
            sta         ptr2 + 1
@entry:     lda         (ptr4)              ; (The table's end: an empty name)
            beq         @noentry
            ldy         #0
@cmp:       lda         (ptr4),y
            cmp         (ptr2),y
            bne         @skip
            iny
            cmp         #0
            bne         @cmp
            lda         (ptr4),y            ; Found: its address after the 0
            pha
            iny
            lda         (ptr4),y
            ldy         #FN_AT + 1
            sta         (ptr3),y
            dey
            pla
            sta         (ptr3),y
            lda         oldbank
            sta         BANKREG
            ldx         slot
            ldy         #FN_SLOT
            txa
            sta         (ptr3),y
            iny
            lda         sgen,x
            sta         (ptr3),y
            jsr         stamp
            clc
            rts
@skip:      ldy         #0                  ; (Past this one: its name, its 0, its address)
:           lda         (ptr4),y
            iny
            cmp         #0
            bne         :-
            iny
            iny
            tya
            clc
            adc         ptr4
            sta         ptr4
            bcc         @entry
            inc         ptr4 + 1
            bra         @entry
@noentry:   lda         oldbank
            sta         BANKREG
            lda         #BE_ENTRY
            sta         _bank_err
            sec
            rts

; stamp: slot .X called now (the calls' count its time)
stamp:
            inc         clock
            bne         :+
            inc         clock + 1
:           lda         clock
            sta         stl,x
            lda         clock + 1
            sta         sth,x
            rts

; findslot: the slot whose child is the file named at ptr1 (the same name: strcmp).  C = 0, .X it; or C = 1
findslot:
            ldx         #0
@slot:      lda         sused,x
            beq         @next
            lda         snl,x
            sta         ptr4
            lda         snh,x
            sta         ptr4 + 1
            ldy         #0
@cmp:       lda         (ptr4),y
            cmp         (ptr1),y
            bne         @next
            iny
            cmp         #0
            bne         @cmp
            clc
            rts
@next:      inx
            cpx         #NSLOT
            bne         @slot
            sec
            rts

; lru: the slot in use called least lately.  OUT: .X
lru:
            ldx         #0
            ldy         #$FF                ; (.Y: the best so far)
@slot:      lda         sused,x
            beq         @next
            cpy         #$FF
            beq         @take
            lda         stl,x               ; (Older than the best: its stamp less)
            cmp         stl,y
            lda         sth,x
            sbc         sth,y
            bcs         @next
@take:      txa
            tay
@next:      inx
            cpx         #NSLOT
            bne         @slot
            tya
            tax
            rts

; ---- load: the child in the file named at ptr1 loaded into a slot's bank (a free slot's new bank, or the one called
; least lately given up for it).  C = 0, .X the slot; or C = 1 (bank_err)
load:
            ldx         #0                  ; A slot not in use, with a bank of its own
@free:      lda         sused,x
            beq         @alloc
            inx
            cpx         #NSLOT
            bne         @free
            jsr         lru                 ; (None: the least lately called given up)
            bra         @give
@alloc:     phx
            lda         #1
            jsr         BANKS_ALLOC
            plx
            bcs         @short
            sta         sbank,x
            bra         @got
@short:     phx                             ; (No bank: one given up, its slot's bank taken)
            jsr         lru
            txa
            ply
            cmp         #$FF
            beq         @nobank
            tax
            lda         sbank,x             ; (Its bank to slot .Y)
            sta         sbank,y
            stz         sused,x
            inc         sgen,x
            tya
            tax
            bra         @got
@give:      cpx         #$FF
            beq         @nobank
            stz         sused,x
            inc         sgen,x
@got:       stx         slot
            lda         ptr1                ; Its name kept
            sta         snl,x
            lda         ptr1 + 1
            sta         snh,x
            lda         BANKREG
            sta         oldbank
            lda         sbank,x
            sta         BANKREG
            jsr         image
            lda         oldbank
            sta         BANKREG
            ldx         slot
            bcs         @failed
            lda         #1
            sta         sused,x
            inc         gencnt
            lda         gencnt
            sta         sgen,x
            lda         iend
            sta         sxl,x
            lda         iend + 1
            sta         sxh,x
            clc
            rts
@failed:    lda         sbank,x             ; (Its bank given back)
            ldx         #1
            jsr         BANKS_FREE
            sec
            rts
@nobank:    lda         #BE_BANK
            sta         _bank_err
            sec
            rts

; ---- image: the child's file (ptr1) read into the bank in the window and made ready.  C = 1 if not (bank_err)
image:
            lda         ptr1
            sta         r0
            lda         ptr1 + 1
            sta         r0 + 1
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         nofile
:           sta         fd
            stz         rpos
            stz         rlen
            ldx         #0                  ; The header
:           jsr         rdbyte
            jcs         short
            sta         hdr,x
            inx
            cpx         #26
            bne         :-
            ldx         #5                  ; Marker 1, 0, "o65", version 0
:           lda         hdr,x
            cmp         o65,x
            jne         format
            dex
            bpl         :-
            lda         hdr + 7             ; Mode: not the 65816's, paged, 32-bit nor an object file
            and         #$F0
            jne         format
            lda         hdr + 8             ; Text at $8000, data after it, BSS after that; zero page at 0
            jne         format
            lda         hdr + 9
            cmp         #>WIN
            jne         format
            clc
            lda         hdr + 8
            adc         hdr + 10
            tax
            lda         hdr + 9
            adc         hdr + 11
            cpx         hdr + 12
            jne         format
            cmp         hdr + 13
            jne         format
            clc
            lda         hdr + 12
            adc         hdr + 14
            tax
            lda         hdr + 13
            adc         hdr + 15
            cpx         hdr + 16
            jne         format
            cmp         hdr + 17
            jne         format
            clc                             ; The image's end: within the bank
            lda         hdr + 16
            adc         hdr + 18
            sta         iend
            lda         hdr + 17
            adc         hdr + 19
            sta         iend + 1
            jcs         format
            cmp         #>WTOP
            jcs         format
            lda         hdr + 20
            ora         hdr + 21
            jne         format
            lda         hdr + 23            ; Its zero page: no more than the caller gives
            jne         zp
            lda         zpn
            cmp         hdr + 22
            jcc         zp
@opt:       jsr         rdbyte              ; Its options: each its length (itself counted), 0 after the last
            jcs         short
            tax
            beq         @body
            dex                             ; (The rest of it)
@skip:      beq         @opt
            jsr         rdbyte
            bcs         short
            dex
            bra         @skip
@body:      lda         #<WIN               ; Its text and data, into the bank
            sta         ptr4
            lda         #>WIN
            sta         ptr4 + 1
            clc
            lda         hdr + 10
            adc         hdr + 14
            sta         cnt
            lda         hdr + 11
            adc         hdr + 15
            sta         cnt + 1
            jsr         rdmany
            bcs         short
            lda         hdr + 18            ; Its BSS cleared (ptr4 at it)
            sta         cnt
            lda         hdr + 19
            sta         cnt + 1
            jsr         clear
            ldx         hdr + 22            ; Its zero page cleared
            beq         :++
            ldy         zpbase
            lda         #0
:           sta         0,y
            iny
            dex
            bne         :-
:           jsr         imports
            bcs         fail
            lda         hdr + 8             ; Its relocations: text's, then data's
            sta         rbase
            lda         hdr + 9
            sta         rbase + 1
            jsr         relocate
            bcs         fail
            lda         hdr + 12
            sta         rbase
            lda         hdr + 13
            sta         rbase + 1
            jsr         relocate
            bcs         fail
            jsr         exports
            bcs         fail
            lda         fd
            jsr         CLOSE
            clc
            rts
short:      lda         #BE_FILE
            bra         err
format:     lda         #BE_FORMAT
            bra         err
zp:         lda         #BE_ZP
err:        sta         _bank_err
fail:       lda         fd
            jsr         CLOSE
            sec
            rts
nofile:     lda         #BE_FILE
            sta         _bank_err
            sec
            rts

; rdmany: cnt bytes of the child's file to ptr4 on (ptr4 past them).  C = 1 at its end
rdmany:
@byte:      lda         cnt
            ora         cnt + 1
            beq         @done
            jsr         rdbyte
            bcs         @end
            sta         (ptr4)
            inc         ptr4
            bne         :+
            inc         ptr4 + 1
:           lda         cnt
            bne         :+
            dec         cnt + 1
:           dec         cnt
            bra         @byte
@done:      clc
@end:       rts

; clear: cnt bytes from ptr4 on made 0 (ptr4 past them)
clear:
@byte:      lda         cnt
            ora         cnt + 1
            beq         @done
            lda         #0
            sta         (ptr4)
            inc         ptr4
            bne         :+
            inc         ptr4 + 1
:           lda         cnt
            bne         :+
            dec         cnt + 1
:           dec         cnt
            bra         @byte
@done:      rts

; rdbyte: the child's file's next byte.  OUT: .A; C = 1 at its end.  Keeps .X, .Y
rdbyte:
            phx
            phy
            lda         rpos
            cmp         rlen
            bcc         @have
            lda         #<rbuf              ; (Its next bytes)
            sta         r0
            lda         #>rbuf
            sta         r0 + 1
            lda         #RBUF
            sta         r1
            stz         r1 + 1
            lda         fd
            jsr         READ
            bcs         @end
            cmp         #0
            beq         @end
            sta         rlen
            stz         rpos
@have:      ldx         rpos
            lda         rbuf,x
            inc         rpos
            ply
            plx
            clc
            rts
@end:       ply
            plx
            sec
            rts

; erdbyte: the exports' file's next byte, the same way (efd, ebuf)
erdbyte:
            phx
            phy
            lda         epos
            cmp         elen
            bcc         @have
            lda         #<ebuf
            sta         r0
            lda         #>ebuf
            sta         r0 + 1
            lda         #RBUF
            sta         r1
            stz         r1 + 1
            lda         efd
            jsr         READ
            bcs         @end
            cmp         #0
            beq         @end
            sta         elen
            stz         epos
@have:      ldx         epos
            lda         ebuf,x
            inc         epos
            ply
            plx
            clc
            rts
@end:       ply
            plx
            sec
            rts

; ---- imports: the child's imports' names (after its BSS: impn), each one's address from the caller's exports (after
; the names: impa, 2 bytes each, $FFFF till found).  C = 1 if one isn't there (bank_err, bank_missing)
imports:
            jsr         rdbyte              ; Their count (255 at most)
            jcs         @short
            sta         nimp
            jsr         rdbyte
            jcs         @short
            cmp         #0
            jne         @format
            lda         iend                ; Their names: each its length's byte (its bytes to the next
            sta         impn                ;   one's: its letters, its 0 and itself, so a name another's
            sta         ptr4                ;   length is passed over at once, match's), then its letters,
            lda         iend + 1            ;   0-ended
            sta         impn + 1
            sta         ptr4 + 1
            ldx         nimp
            beq         @addr
@imp:       lda         ptr4                ; (Its length's byte, its place in ptr2 till it's known)
            sta         ptr2
            lda         ptr4 + 1
            sta         ptr2 + 1
            jsr         inc4
            jcs         @format
            ldy         #1
@name:      jsr         rdbyte
            jcs         @short
            sta         (ptr4)
            iny
            jsr         inc4
            jcs         @format
            cmp         #0
            bne         @name
            tya
            sta         (ptr2)
            dex
            bne         @imp
@addr:      lda         ptr4                ; Their addresses: none found yet
            sta         impa
            lda         ptr4 + 1
            sta         impa + 1
            ldx         nimp
            beq         @exp
:           lda         #$FF
            sta         (ptr4)
            jsr         inc4
            jcs         @format
            lda         #$FF
            sta         (ptr4)
            jsr         inc4
            bcs         @format
            dex
            bne         :-
@exp:       lda         expath              ; The caller's exports: "BX", 1, then each its length, name, address
            sta         r0
            lda         expath + 1
            sta         r0 + 1
            lda         #O_READ
            jsr         OPEN
            bcs         @noexp
            sta         efd
            stz         epos
            stz         elen
            ldx         #0
:           jsr         erdbyte
            bcs         @badexp
            cmp         bxmagic,x
            bne         @badexp
            inx
            cpx         #3
            bne         :-
@each:      jsr         erdbyte             ; An export: its name into ename, its address after it
            bcs         @badexp
            sta         enlen
            cmp         #0
            beq         @done
            cmp         #NAMEMAX + 1
            bcs         @badexp
            ldx         #0
:           jsr         erdbyte
            bcs         @badexp
            sta         ename,x
            inx
            cpx         enlen
            bne         :-
            stz         ename,x
            jsr         erdbyte
            bcs         @badexp
            sta         rval
            jsr         erdbyte
            bcs         @badexp
            sta         rval + 1
            jsr         match               ; (Each import of that name: its address)
            bra         @each
@done:      lda         efd
            jsr         CLOSE
            jmp         missing
@badexp:    lda         efd
            jsr         CLOSE
@noexp:     lda         #BE_EXPORTS
            bra         @err
@short:     lda         #BE_FILE
            bra         @err
@format:    lda         #BE_FORMAT
@err:       sta         _bank_err
            sec
            rts

; inc4: ptr4 on a byte.  C = 1 if it's reached the bank's end.  Keeps .A, .X
inc4:
            inc         ptr4
            bne         :+
            inc         ptr4 + 1
:           pha
            lda         ptr4 + 1
            cmp         #>WTOP
            pla
            rts

; match: each import named ename given the address rval (ptr4 the names, ptr2 the addresses)
match:
            lda         impn
            sta         ptr4
            lda         impn + 1
            sta         ptr4 + 1
            lda         impa
            sta         ptr2
            lda         impa + 1
            sta         ptr2 + 1
            ldx         nimp
            beq         @done
@imp:       lda         (ptr4)              ; (Its length the export's?)
            sec
            sbc         #2
            cmp         enlen
            bne         @no
            ldy         #1
@cmp:       lda         (ptr4),y
            cmp         ename - 1,y
            bne         @no
            iny
            cmp         #0
            bne         @cmp
            lda         rval                ; (The same: its address)
            sta         (ptr2)
            ldy         #1
            lda         rval + 1
            sta         (ptr2),y
@no:        lda         (ptr4)              ; (Past its name: its length)
            clc
            adc         ptr4
            sta         ptr4
            bcc         :+
            inc         ptr4 + 1
:           clc
            lda         ptr2
            adc         #2
            sta         ptr2
            bcc         :+
            inc         ptr2 + 1
:           dex
            bne         @imp
@done:      rts

; missing: an import not found (its address $FFFF): its name into bank_missing, C = 1; else C = 0
missing:
            lda         impn
            sta         ptr4
            lda         impn + 1
            sta         ptr4 + 1
            lda         impa
            sta         ptr2
            lda         impa + 1
            sta         ptr2 + 1
            ldx         nimp
            beq         @ok
@imp:       ldy         #1
            lda         (ptr2),y
            and         (ptr2)
            cmp         #$FF
            beq         @miss
            lda         (ptr4)              ; (Past its name: its length)
            clc
            adc         ptr4
            sta         ptr4
            bcc         :+
            inc         ptr4 + 1
:           clc
            lda         ptr2
            adc         #2
            sta         ptr2
            bcc         :+
            inc         ptr2 + 1
:           dex
            bne         @imp
@ok:        clc
            rts
@miss:      ldy         #1                  ; (Its letters, after its length)
:           lda         (ptr4),y
            sta         _bank_missing - 1,y
            beq         :+
            iny
            cpy         #16
            bne         :-
            lda         #0
            sta         _bank_missing - 1,y
:           lda         #BE_IMPORT
            sta         _bank_err
            sec
            rts

; ---- relocate: a relocation table applied, its places from rbase - 1 on (each its offset from the last, 255 one of
; 254 more; then its type: the value's kind, high 3 bits, and its segment, low 4: an import's index after it, a
; HIGH's low byte after that, but an import's: ld65 writes none), up to its 0.  C = 1 if it's not one this loader takes
relocate:
            lda         rbase
            bne         :+
            dec         rbase + 1
:           dec         rbase
@next:      jsr         rdbyte
            jcs         @short
            cmp         #0
            jeq         @done
            cmp         #255
            bne         @off
            lda         #254
            jsr         addbase
            bra         @next
@off:       jsr         addbase
            jsr         rdbyte
            jcs         @short
            sta         rtype
            and         #$0F                ; The value: an import's address, the zero page's place, or none
            beq         @import
            cmp         #5
            beq         @zero
            stz         rval
            stz         rval + 1
            bra         @apply
@zero:      lda         zpbase
            sta         rval
            stz         rval + 1
            bra         @apply
@import:    jsr         rdbyte              ; (Its index: the import's address, from impa)
            jcs         @short
            sta         rval
            jsr         rdbyte
            jcs         @short
            sta         rval + 1
            asl         rval
            rol         rval + 1
            clc
            lda         impa
            adc         rval
            sta         ptr2
            lda         impa + 1
            adc         rval + 1
            sta         ptr2 + 1
            lda         (ptr2)
            sta         rval
            ldy         #1
            lda         (ptr2),y
            sta         rval + 1
@apply:     lda         rbase
            sta         ptr4
            lda         rbase + 1
            sta         ptr4 + 1
            lda         rtype
            and         #$E0
            cmp         #$80
            beq         @word
            cmp         #$20
            beq         @low
            cmp         #$40
            bne         @format
            lda         rtype               ; HIGH: its low byte after it; but an import's has none (ld65
            and         #$0F                ;   writes none), its offset's low byte taken as 0 (build.js
            beq         @hi0                ;   refuses a child with any other)
            jsr         rdbyte
            bcs         @short
            bra         @hi
@hi0:       lda         #0
@hi:        clc
            adc         rval
            lda         (ptr4)
            adc         rval + 1
            sta         (ptr4)
            jmp         @next
@low:       clc
            lda         (ptr4)
            adc         rval
            sta         (ptr4)
            jmp         @next
@word:      clc
            lda         (ptr4)
            adc         rval
            sta         (ptr4)
            ldy         #1
            lda         (ptr4),y
            adc         rval + 1
            sta         (ptr4),y
            jmp         @next
@done:      clc
            rts
@short:     lda         #BE_FILE
            bra         @err
@format:    lda         #BE_FORMAT
@err:       sta         _bank_err
            sec
            rts

; addbase: rbase on by .A
addbase:
            clc
            adc         rbase
            sta         rbase
            bcc         :+
            inc         rbase + 1
:           rts

; ---- exports: the child's exports (its entries) into its bank after its BSS (iend): each its name, a 0, its
; address; a 0 after the last.  C = 1 if they don't fit
exports:
            jsr         rdbyte
            bcs         @short
            sta         cnt
            jsr         rdbyte
            bcs         @short
            sta         cnt + 1
            lda         iend
            sta         ptr4
            lda         iend + 1
            sta         ptr4 + 1
@each:      lda         cnt
            ora         cnt + 1
            beq         @end
:           jsr         rdbyte              ; Its name, and its 0
            bcs         @short
            sta         (ptr4)
            jsr         inc4
            bcs         @format
            cmp         #0
            bne         :-
            jsr         rdbyte              ; (Its segment: passed over)
            bcs         @short
            jsr         rdbyte              ; Its address
            bcs         @short
            sta         (ptr4)
            jsr         inc4
            bcs         @format
            jsr         rdbyte
            bcs         @short
            sta         (ptr4)
            jsr         inc4
            bcs         @format
            lda         cnt
            bne         :+
            dec         cnt + 1
:           dec         cnt
            bra         @each
@end:       lda         #0
            sta         (ptr4)
            clc
            rts
@short:     lda         #BE_FILE
            bra         @err
@format:    lda         #BE_FORMAT
@err:       sta         _bank_err
            sec
            rts

; ---- void bank_jsr (void): the call through bank_go (bankprog.h's bank_call)
_bank_jsr:
            sta         sava
            stx         savx
            lda         _bank_go
            sta         ptr1
            lda         _bank_go + 1
            sta         ptr1 + 1
            ldy         #FN_SLOT            ; Its child still loaded, the same load?
            lda         (ptr1),y
            tax
            cpx         #NSLOT
            bcs         @again
            lda         sused,x
            beq         @again
            iny
            lda         (ptr1),y
            cmp         sgen,x
            bne         @again
            jsr         stamp
@go:        ldy         #FN_AT
            lda         (ptr1),y
            sta         jvec
            iny
            lda         (ptr1),y
            sta         jvec + 1
            lda         BANKREG
            pha
            lda         sbank,x
            sta         BANKREG
            lda         sava
            ldx         savx
            jsr         @jump
            sta         sava
            pla
            sta         BANKREG
            lda         sava
            rts
@jump:      jmp         (jvec)
@again:     lda         ptr1                ; (Loaded again: the handle made good)
            sta         ptr3
            lda         ptr1 + 1
            sta         ptr3 + 1
            jsr         bind
            bcs         @fail
            lda         ptr3
            sta         ptr1
            lda         ptr3 + 1
            sta         ptr1 + 1
            ldx         slot
            bra         @go
@fail:      lda         #$FF
            tax
            rts

; ---- void bank_flush (void): every child given up
_bank_flush:
            ldx         #0
@slot:      lda         sused,x
            beq         @next
            stz         sused,x
            inc         sgen,x
            phx
            lda         sbank,x
            ldx         #1
            jsr         BANKS_FREE
            plx
@next:      inx
            cpx         #NSLOT
            bne         @slot
            rts

            .rodata
o65:        .byte       1, 0, "o65", 0
bxmagic:    .byte       "BX", 1
