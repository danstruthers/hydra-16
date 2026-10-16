; ****************************************************************************
; bank.s - (dq0p) the main RAM's functions called from the code in the catalogue's bank (things.c's, linked at
; $8000 after the kinds: dq0p.cfg's OV1): each through m0, m1 or m2, which call it and put the catalogue's bank back
; in the window after it (it may have put another there: an item's, the hub's), so the code there goes on.  Its
; arguments as the function wants them (16 bits each: m1's in .A and .X, m2's first on the C stack), its result
; kept through (.A and .X).
;
;   unsigned __fastcall__ m0 (void* f);                         f ()
;   unsigned __fastcall__ m1 (void* f, unsigned a);             f (a)
;   unsigned __fastcall__ m2 (void* f, unsigned a, unsigned b); f (a, b)
;
; And the things' swapped parts (../dq0/swap.inc): code run in the main RAM's top (dq0p.cfg's SWAP), a part at a time,
; copied there as it's called from the bank its images are in (things.c's catalogue reads them in after the kinds:
; dq0p.cfg's OV2); a call of a part's function goes through its stub here (SWFN).

            .export     _m0, _m1, _m2, _ovbank
            .import     popax, pushax, _cat_map

            .include    "zeropage.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 8                     ; Swapped parts' calls in calls, at most

            .bss
fn:         .res        2                   ; The function called
keepa:      .res        1                   ; (.A and .X kept)
keepx:      .res        1
arga:       .res        2                   ; (m2's a)
_ovbank:    .res        1                   ; The parts' images' bank (dq0p.cfg's OV2)
ova:        .res        1                   ; (.A and .X kept through a stub)
ovx:        .res        1

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

; ---- The swapped parts

            .include    "../dq0/swap.inc"

            SWPART      1, 1                ; The store kept, the kit (their images dq0p.cfg's OV2, but in
                                            ;   _ovbank's first, the task's one bank: the catalogue's is the hub's)
            SWPART      2, 1                ; Hidden, a search, a thing described
            SWPART      3, 1                ; Taken up, put down, a tool worn

            SWFN        keep, 1
            SWFN        kit, 1
            SWFN        hide, 2
            SWFN        search, 2
            SWFN        describe, 2
            SWFN        get, 3
            SWFN        drop, 3
            SWFN        wear, 3
