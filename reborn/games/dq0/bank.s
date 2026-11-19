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
                                    ;   2026-11-15)
DEPTH       = 16                    ; Overlay calls in calls, at most
SWDEPTH     = 16                    ; Swapped parts' calls in calls, at most
SWFAST      = 1                     ; (The parts copied four pages a pass: a walk's first words spent half their time
                                    ;   in the copies; 2026-11-03)

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
; overlay 30's, their third (2026-11-15: OV30 linked at $C000), at p less $4000
_said_b:
            sta         ptr1
            txa
            ldy         _ovbank + 27
            cmp         #$A0
            bcc         :+
            ldy         _ovbank + 28
            cmp         #$C0
            bcc         @a0
            ldy         _ovbank + 29
            sbc         #$20               ; (Carry set: past $C000, overlay 30's)
@a0:        sec
            sbc         #$20               ; (Past $A000, overlay 29's)
:           sta         ptr1 + 1
            ldx         BANKREG
            sty         BANKREG
            ldy         #0
            lda         (ptr1),y
            stx         BANKREG
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
            OVFN        title, 1
            OVFN        help, 1
            OVFN        unknown, 24         ; (refuse.c's since 2026-11-06; overlay 24's since 2026-11-07, 1's room)
            OVFN        refused, 11
            OVFN        say_no, 24          ; (The refusals' one table: refuse.c's, step K of COMMANDS-CRITIQUE.md's 5.2)
            OVFN        tt_words, 24        ; (The world's things' coded words worded: refuse.c's, narration as data's stage 5)
            OVFN        parts_in, 6
            OVFN        alias_said, 12      ; (Overlay 4's till 2026-11-10: its room)
            OVFN        sway_said, 15
            OVFN        walk_says, 17       ; (A walk's first words: sway.c's, no part's swap, the user's D17)
            OVFN        blow, 6             ; (A blow worded and its end: blows.c's since 2026-11-03, the main RAM's room)
            OVFN        task_words, 3
            OVFN        stop_kind, 7
            OVFN        answer_of, 7
            OVFN        bar_text, 3
            OVFN        wait_go, 13
            OVFN        time_said, 13
            OVFN        free_cmd, 7
            OVFN        meta, 7
            OVFN        entered, 1
            OVFN        appraise, 1
            OVFN        tell, 14
            OVFN        words, 2
            OVFN        list_say, 16        ; (What's carried told, in pieces: words.c, 2026-11-03)
            OVFN        things_work, 1      ; (The things' timed works, by number: hthings.c, the dispatch table's, §5.2's step L)
            OVFN        verb_row, 11        ; (A verb's row of the dispatch table: actions.c, §5.2's step L, 2026-11-08)
            OVFN        stir_words, 9
            OVFN        den_words, 9
            OVFN        seen_words, 12
            OVFN        pick, 7
            OVFN        choose, 7
            OVFN        resolve, 11
            OVFN        taken_next, 11
            OVFN        throw_said, 10
            OVFN        trap_name, 10
            OVFN        den_way, 4
            OVFN        portal_look, 4
            OVFN        theme_load, 5
            OVFN        theme_named, 5
            OVFN        story, 5
            OVFN        way_words, 5
            OVFN        ending, 15
            OVFN        puzzle, 5
            OVFN        fight_target, 6
            OVFN        fight_note, 22      ; (A fight's word: blows.c's, overlay 6's room, 2026-11-06)
            OVFN        pacing, 16
            OVFN        names_in, 10
            OVFN        focus_words, 13
            OVFN        felt_words, 17
            OVFN        facing_words, 19
            OVFN        turned_says, 19     ; (A turn's first words: task.c's, the main RAM's room)
            OVFN        fore_set, 13        ; (The answer's tasks in the foreground: main.c's)
            OVFN        time_ui, 16         ; (The time of day believed, worded: time.c, 2026-10-31)
            OVFN        say_raw, 16         ; (A line as it is, the map's: term.c, the main RAM's room)
            OVFN        t_status, 19        ; (The status line drawn: term.c, the main RAM's room, 2026-11-03; 19's, 2026-11-11)
            OVFN        status, 19          ; (The status line, as the game is now: main.c's, the main RAM's room, 2026-11-11)
            OVFN        outcome_words, 27   ; (An outcome event worded, its phrase table's row: said.c, ENGINE-CRITIQUE.md's
                                            ;   narration as data, 2026-11-11: dig_words, climb_words, portal_words,
                                            ;   fixt_words and trap_words before it; limb_words, water_words,
                                            ;   harm_words and shot_words, stage 2)
            OVFN        list_words, 27      ; (A sentence with a list worded into lb at lbe: said.c, the look's lists,
                                            ;   2026-11-16)
            OVFN        said_again, 16      ; (A line that says "again": term.c, the verbs plan's B7, 2026-11-10)
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
            OVFN        grapple, 19         ; (Grappling: a grapple's move sent, its words: blows.c, 2026-11-09)
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
