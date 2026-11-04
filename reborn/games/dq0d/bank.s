; ****************************************************************************
; bank.s - (dq0d) the denizens' swapped parts (../dq0/swap.inc): code run in the main RAM's top (dq0d.cfg's SWAP), a
; part at a time, copied there as it's called from the bank its images are in (dq0d.c's ovl_load reads LIBDIR/dq0d.ovl
; into it as the task starts); a call of a part's function goes through its stub here (SWFN).

            .export     _ovbank

            .include    "zeropage.inc"
            .include    "../dq0/ovunpack.inc"    ; (The overlays read and unpacked: ovl_unpack)

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 8                     ; Swapped parts' calls in calls, at most
SWFAST      = 1                     ; (The parts copied four pages a pass: the swap study's, a fifth of the task's copies)

            .zeropage
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

            .bss
_ovbank:    .res        7           ; The parts' images' banks (dq0d.cfg's OV1 ... OV7)

            .include    "../dq0/swap.inc"

            SWPART      1, 1                ; The populations, the placing
            SWPART      2, 1                ; The character's ways with a denizen (the UI's sway), and harmed
            SWPART      3, 2                ; The records paged
            SWPART      4, 2                ; The regions round the character wanted, the records kept; the start
                                            ;   (the kinds read in, popgen loaded);
                                            ;   an actor made, retired, killed, an ally come along
            SWPART      5, 3                ; The senses' own: a hunter's stalk, a scent followed, one drawn on (slice 4.5's)
            SWPART      6, 3                ; Morale: the field and the bonds, an actor's kin dismayed (2026-10-30)
            SWPART      7, 3                ; Succession (2026-10-30)
            SWPART      8, 4                ; Their bodies: their want, the water, a hurt one's rest, a cornered one's
                                            ;   dart (2026-10-31); a lair's home room, a robber's way, one first met
            SWPART      9, 4                ; The day: schedules, sleep, meals, work, watches, leisure (2026-10-31);
                                            ;   the records by it, an actor's plan
            SWPART      10, 5               ; The actors' day kept (their plan, part 9's), their coarse moves, a body
                                            ;   found on the rounds, a pastime let go (2026-10-31)
            SWPART      11, 7               ; The denizens near enlisted (2026-10-31: the main RAM's room; small, cheap
                                            ;   to swap in, not while the UI's command is in hand)
            SWPART      12, 5               ; The tunnellers: a way dug (slice 4.11's, 2026-11-01); their work's,
                                            ;   the builders' (overlay 6's, 2026-11-02)
            SWPART      13, 5               ; The lairs near placed (2026-11-01: the main RAM's room; small, cheap to
                                            ;   swap in, each time round after a move)
            SWPART      14, 5               ; Doors worked (Phase 3C's one path, 2026-11-02)
            SWPART      15, 6               ; The tunnellers' work: mines, burrows; the builders (2026-11-02)
            SWPART      16, 6               ; Gifts, the market, entertaining (2026-11-02: seldom called, apart from the busy parts)
            SWPART      17, 5               ; The ways followed (2026-11-02)
            SWPART      18, 7               ; A way found (2026-11-02)
            SWPART      19, 6               ; A body found on a record's rounds (2026-11-02)
            SWPART      20, 7               ; Hiding oneself (2026-11-04)

            SWFN        populate, 1
            SWFN        place, 1
            SWFN        keep, 4
            SWFN        rec_new, 3
            SWFN        reg_need, 3
            SWFN        dneed, 4
            SWFN        harmed, 2
            SWFN        stalks, 5
            SWFN        scent, 5
            SWFN        draw_on, 5
            SWFN        ally, 5             ; (Slice 4.8's: an ally's errands, one in the way, gifts, regard)
            SWFN        aside, 5
            SWFN        gift, 16            ; (Part 16's since 2026-11-02: a part of its own, the market's beside it)
            SWFN        regard, 5
            SWFN        sway, 2
            SWFN        start, 4
            SWFN        enlist, 4
            SWFN        retire, 4
            SWFN        killed, 4
            SWFN        shoots, 2           ; (A shot at the character: slice 4.7's)
            SWFN        along, 4
            SWFN        dart, 8
            SWFN        hungry, 8
            SWFN        want_fl, 8
            SWFN        fed, 8
            SWFN        depth, 8
            SWFN        rests, 8
            SWFN        fends, 8
            SWFN        home_room, 8
            SWFN        rob_to, 8
            SWFN        met, 8
            SWFN        name_of, 8
            SWFN        kept_file, 8
            SWFN        daily, 10           ; (The day's, 2026-10-31: an actor's, its plan kept)
            SWFN        plan, 9             ; (An actor's day and its place, as it changes: leisure's)
            SWFN        roused, 10          ; (A pastime let go as one rises to act)
            SWFN        hid_mind, 20        ; (Hiding oneself)
            SWFN        enlist_near, 11     ; (The main RAM's room, 2026-10-31)
            SWFN        nearby, 13          ; (The main RAM's room, 2026-11-01: the tunnellers')
            SWFN        dig_way, 12         ; (A tunneller's way dug: slice 4.11's)
            SWFN        work_dig, 15        ; (Its work: a mine, a burrow, a builder's, slice 4.11's rest)
            SWFN        door_way, 14        ; (A door worked: Phase 3C's one path)
            SWFN        take_way, 14        ; (A thing taken up: Phase 3C's one path)
            SWFN        feel, 8             ; (The main RAM's room, 2026-11-02)
            SWFN        robs, 8
            SWFN        shies, 8
            SWFN        way, 17             ; (A way followed: step_for's, 2026-11-02)
            SWFN        wfind, 18           ; (A way found)
            SWFN        slant, 11           ; (A denizen's diagonal way: slant.s, 2026-10-31, the main RAM's room)
            SWFN        wander, 9
            SWFN        sched, 9
            SWFN        coarse, 10
            SWFN        found, 10           ; (A body found on the rounds: the day's)
            SWFN        rfound, 19          ; (A body found on a record's rounds, 2026-11-02)
            SWFN        hour, 3             ; (The hour the game began: the day's, its first)
            SWFN        told, 6             ; (The hook for a loss told: a body found on the rounds, the day's)
            SWFN        dismay, 6
            SWFN        heart, 6
            SWFN        succeed, 7
            SWFN        age, 7               ; (Part 7's since 2026-10-30: part 6's room)
