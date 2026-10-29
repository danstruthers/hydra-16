; ****************************************************************************
; bank.s - Dingus Quest Zero's RAM banks: the window at $8000 set and read; bytes copied between a bank and the
; main RAM; and the overlays' calls.  An overlay is code linked at $8000 (dq0.cfg: OV1 ...), read into a bank of
; its own as the game starts (ovbank: overlay n's bank); a call of an overlay's function goes through its stub here
; (OVFN): the bank there now kept on a little stack of its own, the overlay's put in, its function called, the bank
; put back.  So an overlay can call the main code and other overlays alike, and the main code can call them.  The
; rule that keeps it right: a function here that changes the window (bread, bwrite) never reads or writes the
; caller's memory in it, only the main RAM ($0800-$7EFF).  And the swapped parts' calls (swap.inc): code run in the
; main RAM's top, a part at a time, copied there from its overlay's bank as it's called.

            .export     _setbank, _getbank, _bread, _bwrite, _ovbank
            .import     popa, popax

            .include    "zeropage.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
OVMAX       = 15                    ; dq0.cfg's overlays (the twelfth part 6's image, then seen.c; the
                                    ;   thirteenth part 9's; the fourteenth part 3's, the map: slice 4.2's)
DEPTH       = 16                    ; Overlay calls in calls, at most
SWDEPTH     = 16                    ; Swapped parts' calls in calls, at most

            .bss
_ovbank:    .res        OVMAX       ; Overlay n's bank: _ovbank[n - 1] (main.c's ovl_load sets them)
ovstk:      .res        DEPTH       ; The banks the overlay calls found, to put back
ovsp:       .res        1
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

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

; void __fastcall__ bread (unsigned char bank, const void* from, void* to, unsigned n): n bytes from bank's from
; (in the window) to to (in the main RAM), the window put back as it was
_bread:
            jsr         args                ; ptr1 from, ptr2 to, ptr3 n, tmp1 the bank
            ldy         BANKREG
            lda         tmp1
            sta         BANKREG
            jsr         copy
            sty         BANKREG
            rts

; void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n): n bytes from from (in the
; main RAM) to bank's to (in the window), the window put back as it was
_bwrite:
            jsr         args                ; ptr1 to, ptr2 from, ptr3 n, tmp1 the bank
            lda         ptr1                ; (copy goes from ptr1 to ptr2: swapped)
            ldx         ptr2
            sta         ptr2
            stx         ptr1
            lda         ptr1 + 1
            ldx         ptr2 + 1
            sta         ptr2 + 1
            stx         ptr1 + 1
            ldy         BANKREG
            lda         tmp1
            sta         BANKREG
            jsr         copy
            sty         BANKREG
            rts

; bread's and bwrite's arguments: n (.A, .X) in ptr3, then the two addresses in ptr2 and ptr1, the bank in tmp1
args:
            sta         ptr3
            stx         ptr3 + 1
            jsr         popax
            sta         ptr2
            stx         ptr2 + 1
            jsr         popax
            sta         ptr1
            stx         ptr1 + 1
            jsr         popa
            sta         tmp1
            rts

; ptr3 bytes from ptr1 to ptr2 (upward).  Keeps .Y
copy:
            phy
            ldy         #0
            ldx         ptr3 + 1            ; Whole pages first
            beq         @part
@page:      lda         (ptr1),y
            sta         (ptr2),y
            iny
            bne         @page
            inc         ptr1 + 1
            inc         ptr2 + 1
            dex
            bne         @page
@part:      ldx         ptr3                ; Then what's left
            beq         @done
@byte:      lda         (ptr1),y
            sta         (ptr2),y
            iny
            dex
            bne         @byte
@done:      ply
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
            OVFN        unknown, 1
            OVFN        parts_in, 6
            OVFN        alias_said, 4
            OVFN        sway_words, 6
            OVFN        dig_words, 1
            OVFN        task_words, 3
            OVFN        beat_words, 3
            OVFN        stop_kind, 7
            OVFN        answer_of, 7
            OVFN        bar_text, 3
            OVFN        wait_go, 13
            OVFN        time_said, 13
            OVFN        state_words, 7
            OVFN        free_cmd, 7
            OVFN        meta, 7
            OVFN        climb_words, 1
            OVFN        entered, 1
            OVFN        portal_words, 10
            OVFN        harm_words, 1
            OVFN        appraise, 1
            OVFN        tell, 14
            OVFN        words, 2
            OVFN        more, 2
            OVFN        stir_words, 9
            OVFN        heard_words, 9
            OVFN        den_words, 9
            OVFN        seen_words, 12
            OVFN        pick, 7
            OVFN        choose, 7
            OVFN        answer, 11
            OVFN        resolve, 11
            OVFN        taken_next, 11
            OVFN        throw_words, 10
            OVFN        fixt_words, 4
            OVFN        shot_words, 15      ; (The missiles' words: shots.c, slice 4.7's, overlay 15)
            OVFN        trap_words, 10
            OVFN        trap_name, 10
            OVFN        den_way, 4
            OVFN        portal_look, 4
            OVFN        theme_load, 5
            OVFN        theme_named, 5
            OVFN        story, 5
            OVFN        way_words, 5
            OVFN        ending, 5
            OVFN        puzzle, 5
            OVFN        fight_words, 6
            OVFN        fight_target, 6
            OVFN        pacing, 3
            OVFN        names_in, 10
            OVFN        focus_words, 13
            OVFN        felt_words, 7
            OVFN        facing_words, 7

; ---- The swapped parts (swap.inc; dq0.cfg's SWn: each in the main RAM's area as it's called)

            .include    "swap.inc"

            SWPART      1, 8                ; The start and the end (main.c): the task's arguments, the world
                                            ;   started, the game kept, the world stopped
            SWPART      2, 14               ; The commands (main.c: the map's bank, overlay 3 full at slice 4.2)
            SWPART      3, 14               ; The map (look.c)
            SWPART      4, 9                ; The pace, the watchfulness, the state, small moves (pace.c)
            SWPART      5, 10               ; The names given (names.c)
            SWPART      6, 12               ; The names said (names.c); looking one way, a close look (way.c)
            SWPART      7, 8                ; The game's arguments, its own theme made (main.c)
            SWPART      8, 5                ; Where in its room the character is, and the things (look.c)
            SWPART      9, 13               ; The player's shortcuts (alias.c)
            SWPART      10, 11              ; A search, by its form (search.c)
            SWPART      11, 3               ; The rarer news, worded (main.c); the hands and limbs
            SWPART      12, 15              ; Wounds over time: first aid, in shock, the body's news (tend.c)
            SWPART      13, 1               ; The missiles: a shot, a throw at a denizen, a launcher loaded (main.c)

            SWFN        begin, 1
            SWFN        size, 1
            SWFN        finish, 1
            SWFN        save_game, 1
            SWFN        command, 2
            SWFN        ready, 2
            SWFN        cq_post, 2
            SWFN        cq_cmd, 2
            SWFN        cq_drop, 2
            SWFN        when_free, 2
            SWFN        take, 2
            SWFN        took_up, 2
            SWFN        map, 3
            SWFN        lie_words, 8
            SWFN        room_where, 8
            SWFN        den_seen, 8
            SWFN        actor_seen, 8
            SWFN        pace_cmd, 4
            SWFN        travel_go, 4
            SWFN        names_line, 5
            SWFN        names_den, 6
            SWFN        names_subst, 6
            SWFN        names_say, 6
            SWFN        names_look, 6
            SWFN        names_keep, 6
            SWFN        alias_line, 9
            SWFN        alias_expand, 9
            SWFN        alias_keep, 9
            SWFN        wield, 9
            SWFN        room_named, 6
            SWFN        history, 7
            SWFN        args, 7
            SWFN        reach_go, 7
            SWFN        place_go, 7
            SWFN        look_way, 6
            SWFN        search_go, 10
            SWFN        news, 11
            SWFN        hide, 11
            SWFN        hand_said, 11
            SWFN        limbs_cmd, 11
            SWFN        hold_first, 11
            SWFN        missiles, 13        ; (The missiles: slice 4.7's)
            SWFN        gear_look, 11
            SWFN        early, 12
            SWFN        tend, 12
            SWFN        body_words, 12
