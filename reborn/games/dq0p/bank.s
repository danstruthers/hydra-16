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

            .export     _m0, _m1, _m2
            .import     popax, pushax, _cat_map

            .bss
fn:         .res        2                   ; The function called
keepa:      .res        1                   ; (.A and .X kept)
keepx:      .res        1
arga:       .res        2                   ; (m2's a)

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
