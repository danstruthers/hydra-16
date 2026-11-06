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
            .include    "../dq0/ovunpack.inc"    ; (The overlays read and unpacked: ovl_unpack)

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

            SWPART      1, 22               ; Digging, and the rock it's into (overlay 22's, slice 4.10's)
            SWPART      2, 3                ; Climbing
            SWPART      3, 1                ; The dungeon begun, the strata below grown
            SWPART      4, 2                ; The change records' rarer work (store.c), the UI's requests; a thing of
                                            ;   several, things moved by the count, what closes worked (things.c)
            SWPART      5, 16                ; Doors (wdoor.c)
            SWPART      6, 3                ; Blows (wblow.c)
            SWPART      7, 14               ; Portcullises and windows (wwin.c); the map drawn as it goes (dq0w.c's
                                            ;   draw_test: slice 4.2's)
            SWPART      8, 4                ; Traps and searches (wtrap.c)
            SWPART      9, 4                ; The walks (dq0w.c's go, go_to, step_to)
            SWPART      10, 5               ; The things: the requests answered, a thing handled (things.c)
            SWPART      11, 5               ; The things listed, searched for, described, hidden
            SWPART      12, 20              ; A door's work begun (dq0w.c's door_work)
            SWPART      13, 8               ; The item store's rarer work: begun, kept, the kit, a chunk's things;
                                            ;   the map making, by what the character carries (things.c: part 26's
                                            ;   till 2026-10-28, overlay 9's room)
            SWPART      14, 6               ; The denizens' bodies: their intents carried out (wbody.c)
            SWPART      15, 14              ; The blows, the bleeding (wfight.c)
            SWPART      16, 17              ; The game's own rules: the puzzle judged, a thing walked to (wgame.c)
            SWPART      17, 27              ; The things' files: their chunks paged (ipage.c)
            SWPART      18, 7               ; The change records' files, the save's stamps (wfile.c)
            SWPART      19, 8               ; The game's files begun, the item store kept (wfile.c); a hole dug kept
            SWPART      20, 7               ; The things' files: a chunk's put out, read in (ipage.c)
            SWPART      21, 6               ; The ways across a room worked out (dq0w.c's ways; overlay 5's till 2026-10-28)
            SWPART      22, 17              ; The strata's records out of the hub, back (sfile.c)
            SWPART      23, 11              ; The long task as it goes: a walk a square at a time (dq0w.c's)
            SWPART      24, 12              ; Engagement, a blow's close-in, fighting on (dq0w.c's)
            SWPART      25, 22              ; The engage initiative test, a grope's (dq0w.c's; overlay 6's till 2026-10-28,
                                            ;   13's till 2026-10-31); a dig begun (part 1's room)
            SWPART      26, 15               ; Disengaging (dq0w.c's)
            SWPART      27, 9               ; The lights burning down (dq0w.c's: slice 4.1's)
            SWPART      28, 9               ; The traces' store: the journal taken in, kept (wtrace.c)
            SWPART      29, 10              ; The traces read: the floor, a track, a smell (wtrace.c)
            SWPART      30, 10              ; The traces' store's files: a chunk's records put out, read in (wtrfile.c)
            SWPART      31, 11              ; The traces read, in words (wtrword.c)
            SWPART      32, 1                ; The way the character's scent goes, a hunter's (wtrscent.c)
            SWPART      33, 6              ; A walk across a room (dq0w.c's across: the main RAM's room)
            SWPART      34, 24              ; The rooms' lights by their sconces (wkept.c)
            SWPART      35, 13              ; Things moved by the count, a lot split, joined (things.c: slice 4.1's)
            SWPART      36, 13              ; What closes worked, a tool's wear, a thing described (things.c)
            SWPART      37, 2              ; The rest of the UI's intents carried out (dq0w.c's intent_more)
            SWPART      38, 26              ; A change record kept and made (store.c's store_put, change)
            SWPART      39, 28              ; A thing looked at (things.c: 2026-10-22)
            SWPART      40, 1               ; Its weight, its lock, traps; a thing described (things.c)
            SWPART      41, 11              ; An opening told with what it shows beyond (dq0w.c's revealed; overlay 19's, 2026-10-30: part 72's room)
            SWPART      42, 21              ; What a closer look at a thing shows; its condition (things.c)
            SWPART      43, 7              ; A walk in the dark set out on: its way felt, a thing stumbled over
            SWPART      44, 9              ; A walk in the dark ended: what it met, felt or blundered into
            SWPART      45, 5              ; What's ahead in the dark, felt (dq0w.c's ahead, felt_stop, no_exit)
            SWPART      46, 25               ; A search's thing, for or on; what a search turns up (things.c)
            SWPART      47, 22              ; What's on a thing or in it, searched (things.c)
            SWPART      48, 15              ; The work done and what came of it (dq0w.c's work, worked); the hands
                                            ;   (things.c: slice 4.1b's)
            SWPART      49, 23              ; The limbs: what a hand lets go of, a harm's effects (wlimbs.c; overlay
                                            ;   11's till 2026-11-01)
            SWPART      50, 21              ; The legs: getting up, a move's start prone, how fast; the body's positions,
                                            ;   the steps' loudness (dq0w.c's: overlay 21's, slices 4.5, 4.6)
            SWPART      51, 23              ; The denizens' limbs: a weapon let fall, taken up; legs (wlimbs.c:
                                            ;   overlay 1's bank, overlay 8's room part 19's, 2026-10-27; 5's,
                                            ;   2026-10-30: 19's room the denizens' throws; 23's, 2026-11-01: 5's
                                            ;   room, a tunneller's dig asked)
            SWPART      52, 16              ; The character's bearings underground (dq0w.c's: slice 4.2's)
            SWPART      53, 26              ; A turn, a wait, the quit (dq0w.c's intent_few: slice 4.2's)
            SWPART      54, 16              ; Travel: the way out, as a leg's worked out (dq0w.c's: slice 4.2's)
            SWPART      55, 30             ; Travel to a place it knows (dq0w.c's: slice 4.2's)
            SWPART      56, 17               ; The reach: how readily a thing comes to hand (things.c's, 2026-10-26)
            SWPART      57, 18              ; The outfits drawn: the character's, a denizen's (outfit.c, 2026-10-27)
            SWPART      58, 19              ; The outfits made (outmake.c)
            SWPART      59, 18              ; An outfit's section drawn (outdraw.c)
            SWPART      60, 19              ; The denizens' things; a weapon let fall, taken up (wlimbs.c, 2026-10-27)
            SWPART      61, 8               ; What's in a thing (things.c's s_inside: part 36's room, 2026-10-27)
            SWPART      62, 2               ; A denizen's things made out (things.c's s_gear_words, 2026-10-27)
            SWPART      63, 12              ; The character's kit (things.c's, part 13's till 2026-10-28; overlay 21's, 2026-10-29)
            SWPART      64, 21              ; The coins' mints (things.c's, 2026-10-28; overlay 21's, 2026-10-29)
            SWPART      65, 20              ; The missiles: a shot, its line, where it falls (wshoot.c: slice 4.7's)
            SWPART      66, 23              ; The missiles gathered (wshoot.c; overlay 15's, 2026-10-30)
            SWPART      67, 20              ; The character's shot, the denizens' shots and throws (wshoot.c)
            SWPART      68, 15              ; Where a missile falls, a miss's stray; a launcher loaded (wshoot.c)
            SWPART      69, 26               ; A robbery (wfight.c's: part 67's room, overlay 5's, 2026-10-30; 12's room, 2026-11-02)
            SWPART      70, 25              ; Wounds over time: the bodies' time told, a thing used up (wtend.c; overlay 21's room, slice 4.4)
            SWPART      71, 21              ; A stratum's kept rooms lit as its populations are done late (wkept.c)
            SWPART      72, 28              ; An ally's errands, the character's gifts (wfetch.c: slice 4.8's, 2026-10-30)
            SWPART      73, 12              ; The water: wading, swimming (dq0w.c's: slice 4.9's); the air (2026-10-31)
            SWPART      74, 19              ; A waterskin filled (wwater.c: slice 4.9's)
            SWPART      75, 4               ; The water's depth where the character stands (wwater.c)
            SWPART      76, 23              ; The time of day believed (wtime.c: 2026-10-31)
            SWPART      77, 10              ; The sky seen, the time believed's start (wtime.c)
            SWPART      78, 27             ; Up to a place in the room (dq0w.c's go_to, ways_near: part 9's till
                                            ;   2026-10-31, its room the walk's)
            SWPART      79, 31              ; A tunneller's dig (wdig.c: slice 4.11's, 2026-11-01)
            SWPART      80, 24              ; One path: a denizen's doors, the doors' keys (wport.c: Phase 3C's)
            SWPART      81, 28               ; One path: the take rule, a denizen's take (wtake.c: Phase 3C's)
            SWPART      82, 25              ; A room searched whole (wroom.inc: the user's playtest, 2026-11-02)
            SWPART      83, 25              ; Its tests at a stop (wroom.c)
            SWPART      84, 30              ; The character's dig's own: told why, a hidden door, nothing to show (2026-11-02)
            SWPART      85, 16              ; The reach's steps (things.c's, 2026-11-02: HOUSE.md's reach)
            SWPART      86, 26              ; Their other half
            SWPART      87, 17              ; And the hand the pack goes in, a tool's use, the words
            SWPART      88, 16              ; The hub attached (hub.c's: the start-up's, 2026-11-02)
            SWPART      89, 27              ; A shaft's going, a thing let fall, the daylight's emitters (dq0w.c: 2026-11-03)
            SWPART      90, 13              ; Hands made free, a tool in hand and back (things.c: 2026-11-03)
            SWPART      91, 30              ; A denizen's weapon let fall, or broken (wlimbs.c: 2026-11-03)
            SWPART      92, 5               ; A request of the things served (things.c: part 10's till 2026-11-03)
            SWPART      93, 19              ; An arrival (dq0w.c: the main RAM's till 2026-11-03)
            SWPART      94, 14              ; Hiding oneself (dq0w.c: 2026-11-04)
            SWPART      95, 29              ; A drop found, a rope put in place, a grapple thrown up (wrope3.c)
            SWPART      96, 29              ; Rope and grapples: tied off, let down (wrope.c: 2026-11-04)
            SWPART      97, 29              ; Climbing, a round at a time, a rope's help (dq0w.c: part 2's till 2026-11-04)
            SWPART      98, 30              ; A rope untied, looked at (wrope2.c)
            SWPART      99, 27              ; The time believed drifting: an hour's test, a waking (wtime.c: part 76's till 2026-11-04)
            SWPART      100, 9              ; A light renewed (things.c: part 27's till 2026-11-04)
            SWPART      101, 15              ; A thing hidden, the hands swapped (things.c: parts 13's and 48's till 2026-11-04)
            SWPART      102, 24              ; A thing told apart, what it carries for the UI (things.c, 2026-11-06)
            SWPART      103, 3               ; What it carries listed: gathered first (things.c, 2026-11-06: part 11's room)

            SWFN        time_kept, 99
            SWFN        sky_note, 76
            SWFN        sky_list, 76
            SWFN        work, 48
            SWFN        worked, 48
            SWFN        room_on, 82         ; (A room searched whole: wroom.inc)
            SWFN        room_tests, 83
            SWFN        searched, 37        ; (A search's end: the main RAM's room, 2026-10-31)
            SWFN        outfit, 57
            SWFN        den_kit, 57
            SWFN        outmake, 58
            SWFN        draw_at, 59
            SWFN        hold_in, 48
            SWFN        swap, 101
            SWFN        hands_now, 48
            SWFN        hand_free, 48
            SWFN        doff_feet, 48       ; (A thing taken off, no hand free: at its feet, 2026-11-06)
            SWFN        held_kinds, 48
            SWFN        shand_of, 48
            SWFN        equip_now, 49
            SWFN        drop_held, 49
            SWFN        harm_fx, 49
            SWFN        limb_news, 49
            SWFN        den_reach, 51       ; (Part 60's room, 2026-10-30)
            SWFN        den_took, 60
            SWFN        den_word, 51
            SWFN        den_drop, 91
            SWFN        den_equip, 60
            SWFN        den_legs, 51
            SWFN        den_other, 51
            SWFN        den_dig, 79         ; (A tunneller's dig: slice 4.11's)
            SWFN        pc_dig, 84          ; (The character's dig's own: wdig.c)
            SWFN        den_more, 80        ; (One path: Phase 3C's)
            SWFN        door_key, 80
            SWFN        old_mint, 51
            SWFN        den_enter, 60
            SWFN        den_draw, 60
            SWFN        den_strip, 51
            SWFN        legs, 50
            SWFN        bearings, 52
            SWFN        bearings_off, 52
            SWFN        tsense, 76
            SWFN        sky_ask, 89         ; (The daylight's emitters, a shaft's going, a thing let fall: dq0w.c's)
            SWFN        air_harm, 74
            SWFN        den_way, 53
            SWFN        rest_go, 53
            SWFN        sky_seen, 77
            SWFN        stopped, 40         ; (A trap on the way set off: the main RAM's room, 2026-10-31)
            SWFN        intent_few, 53
            SWFN        way_out, 54
            SWFN        travel_go, 55
            SWFN        travel_on, 55
            SWFN        start_dig, 25
            SWFN        dig_on, 1
            SWFN        climb, 2
            SWFN        climb_on, 97
            SWFN        rope, 96            ; (Rope and grapples: wrope.c)
            SWFN        rope2, 98
            SWFN        rope3, 95
            SWFN        fall, 2
            SWFN        dark_way, 43
            SWFN        dark_end, 44
            SWFN        tripped, 43
            SWFN        ahead, 45
            SWFN        felt_stop, 45
            SWFN        no_exit, 33
            SWFN        unlit_at, 45
            SWFN        wstart, 3
            SWFN        toward_middle, 3
            SWFN        grow_below, 3
            SWFN        start, 3
            SWFN        begin, 3
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
            SWFN        go_to, 78           ; (Part 9's till 2026-10-31: its room the walk's)
            SWFN        walk, 9             ; (A walk along a passage: across's, part 33's; 2026-10-31, the main RAM's room)
            SWFN        serve, 92
            SWFN        tools, 10
            SWFN        step_to, 47
            SWFN        arrive_a, 93
            SWFN        arrive_b, 93
            SWFN        hid_op, 94
            SWFN        light_now, 92
            SWFN        ix_make, 102        ; (What it carries, the UI's first words', 2026-11-06)
            SWFN        inventory, 103      ; (What it carries listed: gathered, then part 11's words, 2026-11-06)
            SWFN        name_apart, 102
            SWFN        tell_apart, 102          ; (A thing told apart by its state, 2026-11-06)
            SWFN        chart_now, 13
            SWFN        at_hand, 4
            SWFN        carried_named, 4
            SWFN        seen_named, 4
            SWFN        pick, 4
            SWFN        moved, 35
            SWFN        shut_work, 36
            SWFN        take_rest, 35
            SWFN        shut_in, 35
            SWFN        bar_hand, 103
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
            SWFN        reach_step, 85      ; (The reach's steps, 2026-11-02)
            SWFN        reach_do, 86
            SWFN        rs_for, 87
            SWFN        rs_more, 90
            SWFN        rs_did2, 75
            SWFN        hub_attach, 88
            SWFN        handy, 56
            SWFN        tool_reach, 72
            SWFN        flame, 87
            SWFN        drop, 35
            SWFN        throw_at, 88
            SWFN        list, 11
            SWFN        wear, 40
            SWFN        make_own, 13
            SWFN        search, 46
            SWFN        seek, 46
            SWFN        on_it, 47
            SWFN        thing_words, 11
            SWFN        describe, 40
            SWFN        look_on, 39
            SWFN        look_pick, 4
            SWFN        look_more, 40
            SWFN        closer, 42
            SWFN        inside, 61
            SWFN        kit_lock, 61
            SWFN        gear_words, 62
            SWFN        listed, 11
            SWFN        many, 103
            SWFN        hide, 101
            SWFN        door_work, 12
            SWFN        batter_on, 12
            SWFN        revealed, 41
            SWFN        work_done, 12
            SWFN        keep, 19
            SWFN        kit, 63
            SWFN        mix_join, 64
            SWFN        mix_words, 64
            SWFN        mix_worth, 64
            SWFN        shot_op, 65         ; (The missiles, slice 4.7's)
            SWFN        gath_op, 66
            SWFN        mfire, 65
            SWFN        pc_shot, 67
            SWFN        mfall, 68
            SWFN        load_go, 68
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
            SWFN        path_ways, 78       ; (Part 78's since 2026-11-01: part 23's room, the walk's runs)
            SWFN        close_go, 24
            SWFN        close_on, 24
            SWFN        zone_step, 24
            SWFN        fight_go, 24
            SWFN        fight_on, 24
            SWFN        disengage_go, 26
            SWFN        flee_on, 26
            SWFN        passed, 26
            SWFN        sconce_lights, 34
            SWFN        kept_wait, 34
            SWFN        kept_late, 71
            SWFN        engaged, 25
            SWFN        fumble_try, 25
            SWFN        den_first, 14
            SWFN        it_flush, 17
            SWFN        st_out, 22
            SWFN        st_in, 22
            SWFN        stratum_in, 3
            SWFN        stock_down, 13
            SWFN        lay_body, 13
            SWFN        den_intents, 14
            SWFN        pc_blow, 15
            SWFN        den_blow, 15
            SWFN        den_rob, 69         ; (Part 15's room, 2026-10-29; 67's, 2026-10-30)
            SWFN        den_fetch, 72       ; (Slice 4.8's)
            SWFN        den_take, 81        ; (One path: Phase 3C's)
            SWFN        take_rule, 81
            SWFN        bled, 15
            SWFN        body_ask, 70
            SWFN        wade, 73            ; (The water, hunger and thirst: slice 4.9's)
            SWFN        water, 74
            SWFN        water_at, 75
            SWFN        use_key, 7
            SWFN        puzzle, 16
            SWFN        take_go, 16
            SWFN        take_up, 16
            SWFN        world_keep, 16
            SWFN        pc_asked, 16
            SWFN        burns_now, 27
            SWFN        task_out, 23
            SWFN        fill, 100
            SWFN        tr_flush, 28
            SWFN        tr_file, 30
            SWFN        traced, 29
            SWFN        tr_words, 31
            SWFN        feet, 31            ; (Its footwear: its prints' tread, part 29's, 2026-11-03)
            SWFN        tr_scent, 32
            SWFN        across, 33
            SWFN        intent_more, 37
            SWFN        on_way, 37
            SWFN        draw_test, 7
            SWFN        store_put, 38
            SWFN        change, 38
