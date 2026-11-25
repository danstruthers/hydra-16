; ****************************************************************************
; jobs.s - (dq0w) the scheduler's walk of dq0w.c's job table (ENGINE-CRITIQUE.md's 7): rows first to end, each its
; trigger asked (jobs_due: 0 none, every pass), then, if it's due, its give-way rule (jobs_held: 0 none, never waits),
; then, if it doesn't wait, its handler (jobs_run).  In assembly for the main RAM's room: cc65's walk was 103 bytes.

            .export     _jobs
            .import     _jobs_due, _jobs_held, _jobs_run, callax

            .bss
row:        .res        1                   ; (The row's offset in the tables, 2 a row)
last:       .res        1                   ; (The rows' end's)

            .code

; void __fastcall__ jobs (unsigned rows): the first row's offset in .A, the end's in .X
_jobs:
            sta         row
            stx         last
@row:
            ldy         row
            cpy         last
            beq         @done
            lda         _jobs_due,y         ; Due?
            ldx         _jobs_due + 1,y
            beq         @due                ; (None: every pass)
            jsr         callax
            tax
            beq         @next
            ldy         row
@due:
            lda         _jobs_held,y        ; Waiting?
            ldx         _jobs_held + 1,y
            beq         @run                ; (None: never)
            jsr         callax
            tax
            bne         @next
            ldy         row
@run:
            lda         _jobs_run,y
            ldx         _jobs_run + 1,y
            jsr         callax
@next:
            inc         row
            inc         row
            bra         @row
@done:
            rts

; unsigned char lockstep (void): the lockstep turns pace (dq0w.c's, its comment there), turns_run's as the character's
; free or asking.  At a real pace, 0.  At the turns pace, 1 if a job of the world's own is due (jobs_due's rows from
; the second), or a chunk the maker stocked not yet put down (the first row's, stocks, every pass's: its slot CS_FRESH,
; a chunk a reader asked for as the pass went on, its things, its sconces' lights and the minute's look at them after
; it): turns_run's 1, the pass ended there, the character's
; E_READY and the rest after its jobs, what they tell told before the prompt, the world's work for that moment of the
; game done before the next line, not under it (lock2's: D17's, a line's first letter not behind the world's minute
; after a climb).  Else 0, and the UI's next line looked to (w_idle not the UI's ui_want): not with a question owed
; (pc_tired), nor (not asking) with its E_READY to come or a move going on (was_busy, move.on; not act.fight's 1, a
; foe that may engage it: there may be no end to that, as it stands by, its fight_on's each pass); then the quiet
; found twice, the events', the denizens' events', the bodies' changes', the denizens' intents' places and ui_want the
; same, the minds' beat on between (rung to it at the first; meanwhile a tick's nap a look, 1, the pass ended, not
; the main loop's four ticks' nap: a line waits on it): w_idle made ui_want, the UI rung.  ui_want read here,
; past turns_run's look at ui_giving and the intents' ring: a thing the UI sets going after that look, its count
; read, is under way by the second finding (ui_giving, an intent in the ring, its news), or done (lock2's, the main
; RAM's room: lock1's ls_w read first).  In assembly, a part of its own (its stub's, parts.txt's): the main RAM's room.
; The hub's header in the window
            .export     _s_lockstep
            .import     _pc_tired, _was_busy, _move, _hub_ring, _hub0, _hub_map, _hub_sleep
            .include    "dq0hub_jobs.inc"       ; (hub.h's: the build's, ../dq0/hubgen.js)

JOBS_END    = 14                                ; (jobs_due's rows, 2 a row: LS_READS)

            .bss
ls_on:      .res        1                       ; (Found quiet once: its places' sum, the minds' beat then)
ls_fp:      .res        1
ls_mb:      .res        1

            .segment    "SW_lockstep_CODE"      ; (Part lockstep: the main RAM's room, lock2's)
_s_lockstep:
            lda         HUB_WINDOW + HUBO_PACE  ; The turns pace alone
            and         #3
            cmp         #3
            beq         @turns
            bra         @zero
@nap:
            lda         #1                      ; (A line waits on it: the next look a tick on, not the main loop's
            jsr         _hub_sleep              ;   nap's four, the minds rung, its 1 the pass ended: lock2's, D17's)
            bra         @one
@owed:
            stz         ls_on
@one:
            jsr         _hub0
            ldx         #0
            lda         #1
            rts
@off:
            stz         ls_on
@zero:
            lda         #0
            tax
            rts
@turns:
            lda         #2                      ; Each job's trigger from the second row's: none due
            sta         row
@row:
            ldy         row
            cpy         #JOBS_END
            beq         @fresh
            lda         _jobs_due,y
            ldx         _jobs_due + 1,y
            jsr         callax
            tax
            bne         @owed
            inc         row
            inc         row
            bra         @row
@fresh:
            lda         #WORLD_BANK             ; No chunk stocked not put down (store.c's stocks)
            jsr         _hub_map
            ldy         #0
@slot:
            lda         HUB_WINDOW + W_CSUSED,y
            cmp         #CS_FRESH
            beq         @owed
            tya
            clc
            adc         #CS_SIZE
            tay
            cpy         #NCSLOTS * CS_SIZE
            bne         @slot
            jsr         _hub0                   ; (The hub's header in the window again)
            lda         HUB_WINDOW + HUBO_WIDLE ; The UI's next line not yet let go
            cmp         HUB_WINDOW + HUBO_WANT
            beq         @zero
            lda         _pc_tired               ; (A question owed)
            bne         @off
            lda         HUB_WINDOW + HUBO_ASKING
            bne         @look                   ; (Asking: the rest no matter)
            lda         _move                   ; (move.on, its first)
            ora         _was_busy
            bne         @off
@look:
            lda         HUB_WINDOW + HUBO_EQ    ; Its places: the same as at the first finding
            clc
            adc         HUB_WINDOW + HUBO_DQ
            adc         HUB_WINDOW + HUBO_SEQ
            adc         HUB_WINDOW + HUBO_DITAIL
            adc         HUB_WINDOW + HUBO_WANT
            ldx         ls_on
            beq         @first
            cmp         ls_fp
            bne         @first
            lda         HUB_WINDOW + HUBO_MBEAT ; And the minds round again since
            cmp         ls_mb
            bne         :+
            jmp         @nap
:
            lda         HUB_WINDOW + HUBO_WANT
            sta         HUB_WINDOW + HUBO_WIDLE
            lda         #T_UI
            jsr         _hub_ring
            jmp         @zero
@first:
            sta         ls_fp
            lda         #1
            sta         ls_on
            lda         HUB_WINDOW + HUBO_MBEAT
            sta         ls_mb
            lda         #T_DENIZENS
            jsr         _hub_ring
            jmp         @nap
