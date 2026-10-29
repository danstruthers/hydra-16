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
SWMANY      = 1                     ; (More than 51 parts: 52 on, slice 4.2's)

            .bss
_ovbank:    .res        23          ; The parts' images' banks (dq0w.cfg's OV1 ...: things.h's OVBANKS), and the
                                    ;   world's own banks (its paging's tables: ipage.c's; the ways across a room;
                                    ;   the traces' store, two: wtrace.c's)
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
            SWPART      2, 3                ; Climbing
            SWPART      3, 1                ; The dungeon begun, the strata below grown
            SWPART      4, 2                ; The change records' rarer work (store.c), the UI's requests; a thing of
                                            ;   several, things moved by the count, what closes worked (things.c)
            SWPART      5, 2                ; Doors (wdoor.c)
            SWPART      6, 3                ; Blows (wblow.c)
            SWPART      7, 14               ; Portcullises and windows (wwin.c); the map drawn as it goes (dq0w.c's
                                            ;   draw_test: slice 4.2's)
            SWPART      8, 4                ; Traps and searches (wtrap.c)
            SWPART      9, 4                ; The walks (dq0w.c's go, go_to, step_to)
            SWPART      10, 5               ; The things: the requests answered, a thing handled (things.c)
            SWPART      11, 5               ; The things listed, searched for, described, hidden
            SWPART      12, 15              ; A door's work begun (dq0w.c's door_work)
            SWPART      13, 8               ; The item store's rarer work: begun, kept, the kit, a chunk's things;
                                            ;   the map making, by what the character carries (things.c: part 26's
                                            ;   till 2026-10-28, overlay 9's room)
            SWPART      14, 6               ; The denizens' bodies: their intents carried out (wbody.c)
            SWPART      15, 14              ; The blows, the bleeding (wfight.c)
            SWPART      16, 17              ; The game's own rules: the puzzle judged, a thing walked to (wgame.c)
            SWPART      17, 6               ; The things' files: their chunks paged (ipage.c)
            SWPART      18, 7               ; The change records' files, the save's stamps (wfile.c)
            SWPART      19, 8               ; The game's files begun, the item store kept (wfile.c); a hole dug kept
            SWPART      20, 7               ; The things' files: a chunk's put out, read in (ipage.c)
            SWPART      21, 6               ; The ways across a room worked out (dq0w.c's ways; overlay 5's till 2026-10-28)
            SWPART      22, 17              ; The strata's records out of the hub, back (sfile.c)
            SWPART      23, 11              ; The long task as it goes: a walk a square at a time (dq0w.c's)
            SWPART      24, 12              ; Engagement, a blow's close-in, fighting on (dq0w.c's)
            SWPART      25, 13              ; The engage initiative test, a grope's (dq0w.c's; overlay 6's till 2026-10-28)
            SWPART      26, 9               ; Disengaging (dq0w.c's)
            SWPART      27, 9               ; The lights burning down (dq0w.c's: slice 4.1's)
            SWPART      28, 9               ; The traces' store: the journal taken in, kept (wtrace.c)
            SWPART      29, 10              ; The traces read: the floor, a track, a smell (wtrace.c)
            SWPART      30, 10              ; The traces' store's files: a chunk's records put out, read in (wtrfile.c)
            SWPART      31, 11              ; The traces read, in words (wtrword.c)
            SWPART      32, 3               ; The way the character's scent goes, a hunter's (wtrscent.c)
            SWPART      33, 12              ; A walk across a room (dq0w.c's across: the main RAM's room)
            SWPART      34, 15              ; The rooms' lights by their sconces (wkept.c)
            SWPART      35, 13              ; Things moved by the count, a lot split, joined (things.c: slice 4.1's)
            SWPART      36, 13              ; What closes worked, a tool's wear, a thing described (things.c)
            SWPART      37, 4               ; The rest of the UI's intents carried out (dq0w.c's intent_more)
            SWPART      38, 16              ; A change record kept and made (store.c's store_put, change)
            SWPART      39, 12              ; A thing looked at (things.c: 2026-10-22)
            SWPART      40, 1               ; Its weight, its lock, traps; a thing described (things.c)
            SWPART      41, 8               ; An opening told with what it shows beyond (dq0w.c's revealed)
            SWPART      42, 2               ; What a closer look at a thing shows; its condition (things.c)
            SWPART      43, 10              ; A walk in the dark set out on: its way felt, a thing stumbled over
            SWPART      44, 15              ; A walk in the dark ended: what it met, felt or blundered into
            SWPART      45, 17              ; What's ahead in the dark, felt (dq0w.c's ahead, felt_stop, no_exit)
            SWPART      46, 7               ; A search's thing, for or on; what a search turns up (things.c)
            SWPART      47, 13              ; What's on a thing or in it, searched (things.c)
            SWPART      48, 15              ; The work done and what came of it (dq0w.c's work, worked); the hands
                                            ;   (things.c: slice 4.1b's)
            SWPART      49, 11              ; The limbs: what a hand lets go of, a harm's effects (wlimbs.c)
            SWPART      50, 14              ; The legs: getting up, a move's start prone, how fast (dq0w.c's)
            SWPART      51, 1               ; The denizens' limbs: a weapon let fall, taken up; legs (wlimbs.c:
                                            ;   overlay 1's bank, overlay 8's room part 19's, 2026-10-27)
            SWPART      52, 16              ; The character's bearings underground (dq0w.c's: slice 4.2's)
            SWPART      53, 16              ; A turn, a wait, the quit (dq0w.c's intent_few: slice 4.2's)
            SWPART      54, 16              ; Travel: the way out, as a leg's worked out (dq0w.c's: slice 4.2's)
            SWPART      55, 16              ; Travel to a place it knows (dq0w.c's: slice 4.2's)
            SWPART      56, 17              ; The reach: how readily a thing comes to hand (things.c's, 2026-10-26)
            SWPART      57, 18              ; The outfits drawn: the character's, a denizen's (outfit.c, 2026-10-27)
            SWPART      58, 19              ; The outfits made (outmake.c)
            SWPART      59, 18              ; An outfit's section drawn (outdraw.c)
            SWPART      60, 19              ; The denizens' things; a weapon let fall, taken up (wlimbs.c, 2026-10-27)
            SWPART      61, 18              ; What's in a thing (things.c's s_inside: part 36's room, 2026-10-27)
            SWPART      62, 17              ; A denizen's things made out (things.c's s_gear_words, 2026-10-27)
            SWPART      63, 12              ; The character's kit (things.c's, part 13's till 2026-10-28)
            SWPART      64, 5               ; The coins' mints (things.c's, 2026-10-28)

            SWFN        work, 48
            SWFN        worked, 48
            SWFN        outfit, 57
            SWFN        den_kit, 57
            SWFN        outmake, 58
            SWFN        draw_at, 59
            SWFN        hold_in, 48
            SWFN        swap, 48
            SWFN        hand_free, 48
            SWFN        held_kinds, 48
            SWFN        shand_of, 48
            SWFN        equip_now, 49
            SWFN        drop_held, 49
            SWFN        harm_fx, 49
            SWFN        limb_news, 49
            SWFN        den_reach, 60
            SWFN        den_took, 60
            SWFN        den_word, 51
            SWFN        den_drop, 60
            SWFN        den_legs, 51
            SWFN        den_other, 60
            SWFN        den_enter, 60
            SWFN        den_draw, 60
            SWFN        den_strip, 60
            SWFN        rise_pc, 50
            SWFN        limbs_go, 50
            SWFN        limbs_pace, 50
            SWFN        bearings, 52
            SWFN        bearings_off, 52
            SWFN        intent_few, 53
            SWFN        way_out, 54
            SWFN        travel_go, 55
            SWFN        travel_on, 55
            SWFN        start_dig, 1
            SWFN        dig_on, 1
            SWFN        climb, 2
            SWFN        climb_on, 2
            SWFN        fall, 2
            SWFN        dark_way, 43
            SWFN        dark_end, 44
            SWFN        tripped, 43
            SWFN        ahead, 45
            SWFN        felt_stop, 45
            SWFN        no_exit, 45
            SWFN        unlit_at, 45
            SWFN        begin, 3
            SWFN        toward_middle, 3
            SWFN        grow_below, 3
            SWFN        start, 3
            SWFN        store_in, 18
            SWFN        store_page, 18
            SWFN        store_save, 18
            SWFN        store_begin, 19
            SWFN        store_commit, 19
            SWFN        dig_kept, 19
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
            SWFN        chart_now, 13
            SWFN        at_hand, 4
            SWFN        carried_named, 4
            SWFN        seen_named, 4
            SWFN        pick, 4
            SWFN        moved, 35
            SWFN        shut_work, 36
            SWFN        take_rest, 35
            SWFN        shut_in, 35
            SWFN        bar_hand, 35
            SWFN        in_reach, 10
            SWFN        weight_in, 10
            SWFN        weight_of, 10
            SWFN        bulk_in, 10
            SWFN        bulk_of, 10
            SWFN        carry, 10
            SWFN        about, 10
            SWFN        reach_from, 56      ; (The reach, 2026-10-26: HOUSE.md's)
            SWFN        reach_to, 56
            SWFN        reach_done, 56
            SWFN        handy, 56
            SWFN        tool_reach, 4
            SWFN        flame, 10
            SWFN        drop, 35
            SWFN        throw_at, 4
            SWFN        list, 11
            SWFN        wear, 40
            SWFN        make_own, 13
            SWFN        seen_owned, 13
            SWFN        search, 46
            SWFN        seek, 46
            SWFN        on_it, 47
            SWFN        thing_words, 11
            SWFN        describe, 40
            SWFN        look_at, 39
            SWFN        look_more, 40
            SWFN        closer, 42
            SWFN        inside, 61
            SWFN        kit_lock, 61
            SWFN        gear_words, 62
            SWFN        listed, 11
            SWFN        many, 35
            SWFN        hide, 13
            SWFN        door_work, 12
            SWFN        batter_on, 12
            SWFN        revealed, 41
            SWFN        work_done, 12
            SWFN        keep, 19
            SWFN        kit, 63
            SWFN        mix_need, 64
            SWFN        mix_join, 64
            SWFN        mix_words, 64
            SWFN        mix_worth, 64
            SWFN        theme_kinds, 61
            SWFN        items_begin, 19
            SWFN        it_room, 17
            SWFN        it_need, 17
            SWFN        it_near, 17
            SWFN        it_out, 20
            SWFN        it_in, 20
            SWFN        ways_go, 21
            SWFN        look_way, 22
            SWFN        step_on, 23
            SWFN        task_news, 23
            SWFN        task_word, 23
            SWFN        path_ways, 23
            SWFN        close_go, 24
            SWFN        close_on, 24
            SWFN        zone_step, 24
            SWFN        fight_go, 24
            SWFN        fight_on, 24
            SWFN        disengage_go, 26
            SWFN        flee_on, 26
            SWFN        sconce_lights, 34
            SWFN        kept_wait, 34
            SWFN        engaged, 25
            SWFN        fumbled, 25
            SWFN        den_first, 14
            SWFN        ways_near, 21
            SWFN        it_flush, 17
            SWFN        st_out, 22
            SWFN        st_in, 22
            SWFN        stratum_in, 3
            SWFN        stock_down, 13
            SWFN        lay_body, 13
            SWFN        den_intents, 14
            SWFN        pc_blow, 15
            SWFN        den_blow, 15
            SWFN        den_rob, 15
            SWFN        bled, 15
            SWFN        puzzle, 16
            SWFN        take_go, 16
            SWFN        take_up, 16
            SWFN        world_keep, 16
            SWFN        pc_asked, 16
            SWFN        burns_now, 27
            SWFN        task_out, 23
            SWFN        fill, 27
            SWFN        tr_flush, 28
            SWFN        tr_file, 30
            SWFN        traced, 29
            SWFN        tr_words, 31
            SWFN        tr_scent, 32
            SWFN        across, 33
            SWFN        intent_more, 37
            SWFN        on_way, 37
            SWFN        draw_test, 7
            SWFN        store_put, 38
            SWFN        change, 38
