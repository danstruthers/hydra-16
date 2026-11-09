; ****************************************************************************
; bank.s - Dingus Quest Zero's RAM banks: the window at $8000 set and read; bytes copied between a bank and the
; and the overlays' calls.  An overlay is code linked at $8000 (dq0.cfg: OV1 ...), read into a bank of
; its own as the game starts (ovbank: overlay n's bank); a call of an overlay's function goes through its stub here
; (OVFN): the bank there now kept on a little stack of its own, the overlay's put in, its function called, the bank
; put back.  So an overlay can call the main code and other overlays alike, and the main code can call them.  The
; rule that keeps it right: a stub that changes the window never reads or writes the caller's memory in it.  And the
; swapped parts' calls (swap.inc): code run in the main RAM's top, a part at a time, copied there from its overlay's
; bank as it's called.  (bread and bwrite gone, 2026-10-30: ovl_load's alone used one, the main RAM's room)

            .export     _setbank, _getbank, _ovbank
            .import     popa, popax

            .include    "zeropage.inc"
            .include    "ovunpack.inc"    ; (The overlays read and unpacked: ovl_unpack)

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
OVMAX       = 24                    ; dq0.cfg's overlays (the twelfth part 6's image, then seen.c; the
                                    ;   thirteenth part 9's; the fourteenth part 3's, the map: slice 4.2's)
DEPTH       = 16                    ; Overlay calls in calls, at most
SWDEPTH     = 16                    ; Swapped parts' calls in calls, at most
SWFAST      = 1                     ; (The parts copied four pages a pass: a walk's first words spent half their time
                                    ;   in the copies; 2026-11-03)

            .zeropage
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

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

; A stub: name() calls overlay ov's o_name(), its arguments and its value as they are (.Y isn't an argument of a
; function that isn't variadic)
.macro      OVFN        name, ov
            .export     .ident(.concat("_", .string(name)))
            .import     .ident(.concat("_o_", .string(name)))
.ident(.concat("_", .string(name))):
            ldy         #ov
            jsr         ovl_in
            jsr         .ident(.concat("_o_", .string(name)))
            jmp         ovl_out
.endmacro

; The overlays' functions (dq0.h has them with their overlays)
            OVFN        title, 1
            OVFN        help, 1
            OVFN        unknown, 24         ; (refuse.c's since 2026-11-06; overlay 24's since 2026-11-07, 1's room)
            OVFN        refused, 24
            OVFN        say_no, 24          ; (The refusals' one table: refuse.c's, step K of COMMANDS-CRITIQUE.md's 5.2)
            OVFN        parts_in, 6
            OVFN        alias_said, 4
            OVFN        sway_words, 15
            OVFN        walk_says, 17       ; (A walk's first words: sway.c's, no part's swap, the user's D17)
            OVFN        blow, 6             ; (A blow worded and its end: blows.c's since 2026-11-03, the main RAM's room)
            OVFN        dig_words, 19
            OVFN        task_words, 3
            OVFN        robbed, 15          ; (A robbery's words: task.c's, overlay 3's room, 2026-11-06)
            OVFN        beat_words, 16          ; (A walk's beats: task.c's, overlay 3's room, 2026-11-01)
            OVFN        stop_kind, 7
            OVFN        answer_of, 7
            OVFN        bar_text, 3
            OVFN        wait_go, 13
            OVFN        time_said, 13
            OVFN        state_words, 7
            OVFN        free_cmd, 7
            OVFN        meta, 7
            OVFN        climb_words, 12     ; (Overlay 1's room, slice 4.4's)
            OVFN        entered, 1
            OVFN        portal_words, 10
            OVFN        harm_words, 12      ; (Overlay 1's till 2026-11-06: a fall into water's words)
            OVFN        appraise, 1
            OVFN        tell, 14
            OVFN        words, 2
            OVFN        list_say, 16        ; (What's carried told, in pieces: words.c, 2026-11-03)
            OVFN        run16, 16           ; (Overlay 16's command handlers, by number: words.c's, the dispatch table's)
            OVFN        verb_row, 11        ; (A verb's row of the dispatch table: actions.c, §5.2's step L, 2026-11-08)
            OVFN        stir_words, 9
            OVFN        den_words, 9
            OVFN        seen_words, 12
            OVFN        pick, 7
            OVFN        choose, 7
            OVFN        resolve, 11
            OVFN        taken_next, 11
            OVFN        throw_words, 10
            OVFN        fixt_words, 4
            OVFN        shot_words, 1       ; (The missiles' words: shots.c, slice 4.7's, overlay 15)
            OVFN        trap_words, 10
            OVFN        trap_name, 10
            OVFN        den_way, 4
            OVFN        portal_look, 4
            OVFN        theme_load, 5
            OVFN        theme_named, 5
            OVFN        story, 5
            OVFN        way_words, 5
            OVFN        ending, 15
            OVFN        puzzle, 5
            OVFN        fight_words, 6
            OVFN        fight_target, 6
            OVFN        fight_note, 22      ; (A fight's word: blows.c's, overlay 6's room, 2026-11-06)
            OVFN        pacing, 16
            OVFN        names_in, 10
            OVFN        focus_words, 13
            OVFN        felt_words, 17
            OVFN        facing_words, 19
            OVFN        sky_words, 1        ; (The sky seen by a window, a skylight, a shaft: time.c, 16's room)
            OVFN        turned_says, 19     ; (A turn's first words: task.c's, the main RAM's room)
            OVFN        fore_set, 13        ; (The answer's tasks in the foreground: main.c's)
            OVFN        water_words, 4      ; (The water's news, hunger's and thirst's: water.c, slice 4.9's)
            OVFN        time_ui, 16         ; (The time of day believed, worded: time.c, 2026-10-31)
            OVFN        say_raw, 16         ; (A line as it is, the map's: term.c, the main RAM's room)
            OVFN        t_status, 16        ; (The status line drawn: term.c, the main RAM's room, 2026-11-03)
            OVFN        may_den, 14         ; (The thing named may be a denizen: sway.c, 2026-11-03)
            OVFN        place_said, 14      ; ("From the floor": a place, not a thing, 2026-11-06)
            OVFN        walk_ahead, 20      ; (A command's first words by what it carries: sway.c, 2026-11-06)
            OVFN        letters_in, 22      ; (A line's letters made its things' names: letters.c, 2026-11-06)
            OVFN        what_ask, 22        ; (A command with nothing named asked about: letters.c)
            OVFN        des_thing, 22       ; (A thing a letter named, a denizen's found: letters.c)
            OVFN        des_seen, 22        ; (A thing the resolver chose, by its id: letters.c, 2026-11-07)
            OVFN        des_add, 24         ; ("It"'s thing, "take all"'s, by its id: letters.c, step B's, 2026-11-07)
            OVFN        des_low, 22
            OVFN        des_req, 22
            OVFN        cap_of, 23          ; (A thing in sight's capital, the look's: caps.c, overlay 22's till 2026-11-06)
            OVFN        around, 23          ; (What's in sight listed: caps.c)
            OVFN        den_caps, 23        ; (The denizens' capitals, the look's: caps.c)
            OVFN        which_seen, 24      ; (The resolver's question, by capitals: which.c, 2026-11-07)
            OVFN        cap_index, 23       ; (A thing's or a denizen's capital: caps.c, for which.c)
            OVFN        which_way, 24       ; (An answer by where its choice lies: which.c, 2026-11-07)
            OVFN        which_put, 24       ; (A door's or an opening's choice for the question: which.c)
            OVFN        which_place, 24     ; (choose.c's question made the one which_in answers: which.c)
            OVFN        which_ask, 23       ; (A take's which asked: caps.c, the designators' step 5)
            OVFN        den_heard, 23       ; (A denizen's sound weighed: main.c's heard, the main RAM's room)
            OVFN        heard_words, 23     ; (A denizen heard: caps.c, overlay 9's till 2026-11-06, its room for the capitals)
            OVFN        dots, 3            ; (The game's start's progress: main.c's)

; ---- The swapped parts (swap.inc; dq0.cfg's SWn: each in the main RAM's area as it's called)

            .include    "swap.inc"

            .include    "parts.inc"         ; (The parts and their stubs, the build's: parts.txt, ../dq0/parts.js)
