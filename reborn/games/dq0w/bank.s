; ****************************************************************************
; bank.s - (dq0w) the world's swapped parts (../dq0/swap.inc): code run in the main RAM's top (dq0w.cfg's SWAP), a
; part at a time, copied there as it's called from the bank its image is in (dq0w.c's ovl_load reads LIBDIR/dq0w.ovl
; into them as the task starts: dq0w.cfg's OV1 ...); a call of a part's function goes through its stub here (SWFN).
;
; And the main RAM's functions called from the code in the catalogue's bank (things.c's, linked at $8000 after the
; kinds: dq0w.cfg's CAT, read into the hub's catalogue bank as the world starts): each through m0, m1 or m2, which
; call it and put the catalogue's bank back in the window after it (it may have put another there: an item's, the
; hub's), so the code there goes on.  Its arguments as the function wants them (16 bits each: m1's in .A and .X, m2's
; first on the C stack), its result kept through (.A and .X).  A part's function (part 10's: the catalogue's code
; runs while part 10's in the area, called from it) through its stub.
;
;   unsigned __fastcall__ m0 (void* f);                         f ()
;   unsigned __fastcall__ m1 (void* f, unsigned a);             f (a)
;   unsigned __fastcall__ m2 (void* f, unsigned a, unsigned b); f (a, b)

            .export     _ovbank, _m0, _m1, _m2
            .import     popax, pushax, _cat_map

            .include    "zeropage.inc"
OVL_BANKS   = OVBANKS + 4           ; (The game's library, ../dq0/dq0lib.inc: ovl_load, num; the overlays and
OVL_DOT     = 0                     ;   the world's own four banks)
.define     OVL_FILE "/dq0w.ovl"
            .include    "../dq0/dq0lib.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 12                    ; Swapped parts' calls in calls, at most
SWFAST      = 1                     ; (The parts copied four pages a pass: an answer's part, the list's, the climb's, the
                                    ;   quicker; 2026-10-30)
SWMANY      = 1                     ; (More than 51 parts: 52 on, slice 4.2's)
OVBANKS     = 31                    ; The overlays (dq0w.cfg's OV1 ...): things.h's OVBANKS (make.js checks the three agree)

            .zeropage
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

            .bss
_ovbank:    .res        OVBANKS + 4 ; The parts' images' banks (dq0w.cfg's OV1 ...: things.h's OVBANKS), and the
                                    ;   world's own banks (its paging's tables: ipage.c's; the ways across a room;
                                    ;   the traces' store, two: wtrace.c's)
fn:         .res        2           ; (m0 ...: the function called,
keepa:      .res        1           ;   .A and .X kept,
keepx:      .res        1
arga:       .res        2           ;   m2's a)

            .code

_m0:
            sta         fn
            stx         fn + 1
            jmp         call

_m1:
            sta         keepa               ; (a)
            stx         keepx
            jsr         popax               ; (f)
            sta         fn
            stx         fn + 1
            lda         keepa
            ldx         keepx
            jmp         call

_m2:
            sta         keepa               ; (b)
            stx         keepx
            jsr         popax               ; (a)
            sta         arga
            stx         arga + 1
            jsr         popax               ; (f)
            sta         fn
            stx         fn + 1
            lda         arga                ; (a back on the C stack, b in .A and .X)
            ldx         arga + 1
            jsr         pushax
            lda         keepa
            ldx         keepx

; The function called, then the catalogue's bank put back, its result kept
call:
            jsr         go
            sta         keepa
            stx         keepx
            jsr         _cat_map
            lda         keepa
            ldx         keepx
            rts
go:
            jmp         (fn)

            .include    "../dq0/swap.inc"

            .include    "parts.inc"         ; (The parts and their stubs, the build's: parts.txt, ../dq0/parts.js)
