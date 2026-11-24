; ****************************************************************************
; bank.s - Dingus Quest Zero's RAM banks: the window at $8000 set and read; bytes copied between a bank and the
; and the overlays' calls.  An overlay is code linked at $8000 (dq0.cfg: OV1 ...), read into a bank of
; its own as the game starts (ovbank: overlay n's bank); a call of an overlay's function goes through its stub here
; (OVFN): the bank there now kept on a little stack of its own, the overlay's put in, its function called, the bank
; put back.  So an overlay can call the main code and other overlays alike, and the main code can call them.  The
; rule that keeps it right: a stub that changes the window never reads or writes the caller's memory in it.  And the
; swapped parts' calls (swap.inc): code run in the main RAM's top, a part at a time, copied there from its overlay's
; bank as it's called.  (bread and bwrite gone, 2026-10-30: ovl_load's alone used one, the main RAM's room)

            .export     _setbank, _getbank, _ovbank, _said_b
            .import     popa, popax

            .include    "zeropage.inc"
OVL_BANKS   = OVMAX                 ; (The game's library, dq0lib.inc: the overlays' banks, a '.' after the first
OVL_DOT     = 12                    ;   twelve, the screen's silence under its 36M cycles; the file find_dirs'
.define     OVL_FILE ""             ;   opened: ovl_load's, num's, dnum's)
            .include    "dq0lib.inc"
            .export     _dnum
_dnum       = _num

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
OVMAX       = OVLS                  ; dq0.cfg's overlays, the build's count (the twelfth part 6's image, then seen.c; the
                                    ;   thirteenth part 9's; the fourteenth part 3's, the map: slice 4.2's;
                                    ;   the 25th part 27's; the 26th part 28's, 2026-11-10; the
                                    ;   27th the status line, 2026-11-11; the 28th the phrase table's rows,
                                    ;   said.txt's, 2026-11-11: said_b; the 29th
                                    ;   their second bank, stage 3; the 30th their third,
                                    ;   2026-11-15; the 31st their fourth, 2026-11-24)
DEPTH       = 16                    ; Overlay calls in calls, at most
SWDEPTH     = 16                    ; Swapped parts' calls in calls, at most
SWFAST      = 1                     ; (The parts copied four pages a pass: a walk's first words spent half their time
                                    ;   in the copies; 2026-11-03)
SWSTUBOV    = 15                    ; (The stubs' tables in overlay 15's bank, dq0.cfg's SWSTP ...: a stub 4 bytes, not
                                    ;   6, the main RAM's room, room8, 2026-11-24: room7's open question 1)

            .zeropage
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1
ovip:       .res        2           ; (Where a stub's numbers are: ovcall's)

            .bss
_ovbank:    .res        OVMAX       ; Overlay n's bank: _ovbank[n - 1] (main.c's ovl_load sets them)
ovstk:      .res        DEPTH       ; The banks the overlay calls found, to put back
ovsp:       .res        1

            .code

; void __fastcall__ setbank (unsigned char b): the bank at $8000.  (hydra.h's hy_bank, but a call: cc65's optimizer
; can lose an index in hy_bank (a[i]))
_setbank:
            sta         BANKREG
            rts

; unsigned char getbank (void): the bank at $8000
_getbank:
            lda         BANKREG
            ldx         #0
            rts

; unsigned char __fastcall__ said_b (const unsigned char* p): byte p of overlay 28's bank (the phrase table's rows,
; said.c's: its own bank, overlay 27's room for the renderer), the window put back as it was.  (Some 40 cycles a byte
; with its call: a sentence's few hundred bytes read, a row found by the rows' lengths).  p past $A000, overlay 29's,
; the rows' second bank (stage 3, 2026-11-11: dq0.cfg's OV29 linked at $A000), at p less $2000 in it; past $C000,
; overlay 30's, their third (2026-11-15: OV30 linked at $C000), at p less $4000; past $E000, overlay 31's, their fourth
; (room8, 2026-11-24: OV31 linked at $E000), at p less $6000.  (The first two banks' bytes as quick as before)
_said_b:
            sta         ptr1
            txa
            ldy         _ovbank + 27
            cmp         #$A0
            bcc         @in
            ldy         _ovbank + 28
            cmp         #$C0
            bcc         @a0
            ldy         _ovbank + 29
            cmp         #$E0
            bcs         @e0
            sbc         #$1F               ; (Carry clear: past $C000, overlay 30's, less $20)
@a0:        sec
            sbc         #$20               ; (Past $A000, overlay 29's)
@in:        sta         ptr1 + 1
            ldx         BANKREG
            sty         BANKREG
            ldy         #0
            lda         (ptr1),y
            stx         BANKREG
            ldx         #0
            rts
@e0:        ldy         _ovbank + 30
            sbc         #$60               ; (Carry set: past $E000, overlay 31's)
            bra         @in

; ---- The overlays' calls

; Overlay .Y's bank in the window, the one there kept.  Keeps .A, .X
ovl_in:
            sta         ova
            stx         ovx
            ldx         ovsp
            lda         BANKREG
            sta         ovstk,x
            inc         ovsp
            lda         _ovbank - 1,y
            sta         BANKREG
            lda         ova
            ldx         ovx
            rts

; The bank the last ovl_in found, put back.  Keeps .A, .X (a function's value)
ovl_out:
            sta         ova
            stx         ovx
            dec         ovsp
            ldx         ovsp
            lda         ovstk,x
            sta         BANKREG
            lda         ova
            ldx         ovx
            rts

; A stub's call (its jsr's: the overlay's number and the function's address after it): the overlay put in, its
; function called, the bank put back, its arguments and its value as they are.  (A stub its call and its numbers, 6
; bytes, as the swapped parts' are: 11 before, its own ldy, jsr ovl_in, jsr and jmp ovl_out; some 30 cycles a call
; more for some 430 bytes of the main RAM, 2026-11-11)
ovcall:
            sta         ova
            stx         ovx
            pla                             ; (The stub's jsr: the numbers follow it)
            sta         ovip
            pla
            sta         ovip + 1
            ldy         #2
            lda         (ovip),y
            sta         @go + 1
            iny
            lda         (ovip),y
            sta         @go + 2
            ldx         ovsp                ; (ovl_in's, here: the bank there kept,
            lda         BANKREG
            sta         ovstk,x
            inc         ovsp
            ldy         #1                  ;   the overlay's put in)
            lda         (ovip),y
            tay
            lda         _ovbank - 1,y
            sta         BANKREG
            lda         ova
            ldx         ovx
@go:        jsr         $FFFF
            jmp         ovl_out

; A stub: name() calls overlay ov's o_name(), its arguments and its value as they are (.Y isn't an argument of a
; function that isn't variadic)
.macro      OVFN        name, ov
            .export     .ident(.concat("_", .string(name)))
            .import     .ident(.concat("_o_", .string(name)))
.ident(.concat("_", .string(name))):
            jsr         ovcall
            .byte       ov
            .addr       .ident(.concat("_o_", .string(name)))
.endmacro

; The overlays' functions (dq0.h has them with their overlays)
            .include    "ovfn.inc"          ; (Each o_NAME an overlay's C defines and something calls: its stub, NAME,
                                            ;   the build's: ../dq0/parts.js's ovstubs)

; ---- The swapped parts (swap.inc; dq0.cfg's SWn: each in the main RAM's area as it's called)

            .include    "swap.inc"

            .include    "parts.inc"         ; (The parts and their stubs, the build's: parts.txt, ../dq0/parts.js)
