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

; void lockstep (void): the lockstep turns pace's quiet (dq0w.c's, its comment there; turns_run's, the character free or
; asking, with nothing to do): not at a real pace, nor with a question owed (pc_tired), nor (not asking) with its E_READY
; to come or a move going on (was_busy, move.on; not act.fight's 1, a foe that may engage it: there may be no end to
; that, as it stands by, its fight_on's each pass), nor with a job of its own due (jobs_due's rows
; from the second); then found so twice, the events', the denizens' events', the bodies' changes' and the denizens'
; intents' places the same, the minds' beat on between (rung to it at the first): w_idle made ls_w, the UI rung.  In
; assembly, in a part of its own (its stub's, parts.txt's): the main RAM's room.  The hub's header in the window
            .export     _s_lockstep, _ls_w
            .import     _pc_tired, _was_busy, _move, _hub_ring, _hub0
            .include    "dq0hub_jobs.inc"       ; (hub.h's: the build's, ../dq0/hubgen.js)

JOBS_END    = 14                                ; (jobs_due's rows, 2 a row: LS_READS)

            .bss
_ls_w:      .res        1                       ; (The UI's ui_want as turns_run read it first)
ls_on:      .res        1                       ; (Found quiet once: its places' sum, the minds' beat then)
ls_fp:      .res        1
ls_mb:      .res        1

            .segment    "SW_lockstep_CODE"      ; (Part lockstep: the main RAM's room, lock2's)
_s_lockstep:
            lda         HUB_WINDOW + HUBO_PACE  ; The turns pace alone
            and         #3
            cmp         #3
            bne         @off
            lda         _pc_tired               ; (A question owed)
            bne         @off
            lda         HUB_WINDOW + HUBO_ASKING
            bne         @jobs                   ; (Asking: the rest no matter)
            lda         _move                   ; (move.on, its first)
            ora         _was_busy
            bne         @off
@jobs:
            lda         #2                      ; Each job's trigger from the second row's: none due
            sta         row
@row:
            ldy         row
            cpy         #JOBS_END
            beq         @quiet
            lda         _jobs_due,y
            ldx         _jobs_due + 1,y
            jsr         callax
            tax
            bne         @off
            inc         row
            inc         row
            bra         @row
@quiet:
            jsr         _hub0                   ; (The hub's header in the window again)
            lda         HUB_WINDOW + HUBO_EQ    ; Its places: the same as at the first finding
            clc
            adc         HUB_WINDOW + HUBO_DQ
            adc         HUB_WINDOW + HUBO_SEQ
            adc         HUB_WINDOW + HUBO_DITAIL
            ldx         ls_on
            beq         @first
            cmp         ls_fp
            bne         @first
            lda         HUB_WINDOW + HUBO_MBEAT ; And the minds round again since
            cmp         ls_mb
            beq         @done
            lda         _ls_w
            sta         HUB_WINDOW + HUBO_WIDLE
            lda         #T_UI
            jmp         _hub_ring
@first:
            sta         ls_fp
            lda         #1
            sta         ls_on
            lda         HUB_WINDOW + HUBO_MBEAT
            sta         ls_mb
            lda         #T_DENIZENS
            jmp         _hub_ring
@off:
            stz         ls_on
@done:
            rts
