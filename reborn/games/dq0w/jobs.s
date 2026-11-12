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
