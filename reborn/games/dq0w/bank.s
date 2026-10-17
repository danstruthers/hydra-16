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

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 8                     ; Swapped parts' calls in calls, at most

            .bss
_ovbank:    .res        10          ; The parts' images' banks (dq0w.cfg's OV1 ...: things.h's OVBANKS), and the
                                    ;   world's own banks (its paging's tables: ipage.c's; the ways across a room)
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1
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

            SWPART      1, 1                ; Digging
            SWPART      2, 1                ; Climbing
            SWPART      3, 1                ; The dungeon begun, the strata below grown
            SWPART      4, 2                ; The change records' rarer work (store.c), the UI's requests
            SWPART      5, 2                ; Doors (wdoor.c)
            SWPART      6, 3                ; Blows (wblow.c)
            SWPART      7, 3                ; Portcullises and windows (wwin.c)
            SWPART      8, 4                ; Traps and searches (wtrap.c)
            SWPART      9, 4                ; The walks (dq0w.c's go, go_to, step_to)
            SWPART      10, 5               ; The things: the requests answered, a thing handled (things.c)
            SWPART      11, 5               ; The things listed, searched for, described, hidden
            SWPART      12, 4               ; A door's work begun (dq0w.c's door_work)
            SWPART      13, 6               ; The item store's rarer work: begun, kept, the kit, a chunk's things
            SWPART      14, 6               ; The denizens' bodies: their intents carried out (wbody.c)
            SWPART      15, 2               ; The blows, the bleeding (wfight.c)
            SWPART      16, 6               ; The game's own rules: the puzzle judged, a thing walked to (wgame.c)
            SWPART      17, 8               ; The things' files: their chunks paged (ipage.c)
            SWPART      18, 7               ; The change records' files, the save's stamps (wfile.c)
            SWPART      19, 8               ; The game's files begun, the item store kept (wfile.c)
            SWPART      20, 7               ; The things' files: a chunk's put out, read in (ipage.c)
            SWPART      21, 5               ; The ways across a room worked out (dq0w.c's ways)

            SWFN        start_dig, 1
            SWFN        dig_on, 1
            SWFN        climb, 2
            SWFN        climb_on, 2
            SWFN        begin, 3
            SWFN        grow_below, 3
            SWFN        start, 3
            SWFN        store_in, 18
            SWFN        store_page, 18
            SWFN        store_save, 18
            SWFN        store_begin, 19
            SWFN        store_commit, 19
            SWFN        answer, 4
            SWFN        stocked, 4
            SWFN        doors, 5
            SWFN        blows, 6
            SWFN        portals, 7
            SWFN        traps, 8
            SWFN        go, 9
            SWFN        go_to, 9
            SWFN        step_to, 9
            SWFN        serve, 10
            SWFN        named, 10
            SWFN        on_pc, 10
            SWFN        light_now, 10
            SWFN        at_hand, 10
            SWFN        carried_named, 10
            SWFN        seen_named, 10
            SWFN        in_reach, 10
            SWFN        weight_in, 10
            SWFN        weight_of, 10
            SWFN        bulk_in, 10
            SWFN        bulk_of, 10
            SWFN        carry, 10
            SWFN        about, 10
            SWFN        drop, 10
            SWFN        list, 11
            SWFN        wear, 11
            SWFN        make_own, 13
            SWFN        seen_owned, 13
            SWFN        search, 11
            SWFN        describe, 11
            SWFN        hide, 13
            SWFN        door_work, 12
            SWFN        keep, 19
            SWFN        kit, 13
            SWFN        theme_kinds, 13
            SWFN        items_begin, 19
            SWFN        it_room, 17
            SWFN        it_need, 17
            SWFN        it_near, 17
            SWFN        it_out, 20
            SWFN        it_in, 20
            SWFN        ways_go, 21
            SWFN        stock_down, 13
            SWFN        lay_body, 13
            SWFN        den_intents, 14
            SWFN        pc_blow, 15
            SWFN        den_blow, 15
            SWFN        bled, 15
            SWFN        puzzle, 16
            SWFN        take_go, 16
            SWFN        take_up, 16
            SWFN        world_keep, 16
            SWFN        pc_asked, 16
