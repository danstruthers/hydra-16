; ****************************************************************************
; signal.s - signal() over the Hydra's notes (Plan 9's): a note handler (NOTIFY, set by a constructor, so with
; signal() in the program) takes each note the task gets, and runs the handler signal() gave its signal (cc65's
; sigtable): the interrupt note (Ctrl-C at its window) is SIGINT, the hangup SIGTERM, a BRK SIGILL.  SIG_DFL: the
; note's default (the task ends, the note's name its status); SIG_IGN: it goes on; a handler: it runs, then the
; task goes on where it was (where the note found it: never in the middle of a system call).  The runtime's zero
; page is kept around the handler, and it has a C stack of its own, so the code the note stopped finds both as it
; left them; a handler is best kept small (a flag set).  Any other note: its default.  (It takes the place of
; cc65's signal module: signal() is the same.)

            .export     _signal, ___sig_dfl, ___sig_ign
            .constructor initnotes, 20
            .import     sigtable, popax, ___seterrno

            .include    "zeropage.inc"
            .include    "signal.inc"
            .include    "errno.inc"
            .include    "hydra.inc"

SIGSTACK        = 256           ; A handler's C stack

            .segment    "ONCE"

initnotes:
            lda         #<handler
            sta         r0
            lda         #>handler
            sta         r0 + 1
            jmp         NOTIFY

            .code

; __sigfunc __fastcall__ signal (int sig, __sigfunc func): sig's handler set (a note may come meanwhile: the IRQs
; off for the table's two bytes).  OUT: the old handler, or SIG_ERR (0) and errno EINVAL
_signal:
            sta         ptr1                                ; The handler
            stx         ptr1 + 1
            jsr         popax                               ; The signal
            cpx         #0
            bne         inval
            cmp         #SIGCOUNT
            bcs         inval
            asl
            tax
            sei
            lda         sigtable,X
            pha
            lda         ptr1
            sta         sigtable,X
            lda         sigtable + 1,X
            pha
            lda         ptr1 + 1
            sta         sigtable + 1,X
            cli
            pla
            tax
            pla
___sig_ign:
            rts

inval:
            lda         #<EINVAL
            jsr         ___seterrno                         ; (.A = 0)
            tax
___sig_dfl:
            rts

; The note handler: .A = the note.  OUT: C = 0, the task goes on; C = 1, the note's default
handler:
            ldx         #NOTES - 1                          ; Its signal
:
            cmp         notes,X
            beq         :+
            dex
            bpl         :-
            sec                                             ; (None: the default)
            rts
:
            lda         signals,X
            sta         sig
            asl                                             ; Its handler, signal()'s
            tax
            lda         sigtable,X
            sta         vec
            lda         sigtable + 1,X
            sta         vec + 1
            cmp         #>___sig_dfl
            bne         :+
            lda         vec
            cmp         #<___sig_dfl
            bne         :+
            sec                                             ; (SIG_DFL: the default)
            rts
:
            lda         vec + 1
            cmp         #>___sig_ign
            bne         @call
            lda         vec
            cmp         #<___sig_ign
            bne         @call
            clc                                             ; (SIG_IGN: on)
            rts

@call:
            ldx         #zpspace - 1                        ; The runtime's zero page kept ...
:
            lda         sp,X
            sta         zpkeep,X
            dex
            bpl         :-
            lda         #<(stack + SIGSTACK)                ;   a C stack of its own ...
            sta         sp
            lda         #>(stack + SIGSTACK)
            sta         sp + 1
            lda         sig                                 ;   the handler (int sig) ...
            ldx         #0
            jsr         go
            ldx         #zpspace - 1                        ;   and the zero page as it was
:
            lda         zpkeep,X
            sta         sp,X
            dex
            bpl         :-
            clc
            rts

go:
            jmp         (vec)

            .rodata
notes:      .byte       NOTE_INTERRUPT, NOTE_HANGUP, NOTE_BRK
NOTES           = * - notes
signals:    .byte       SIGINT, SIGTERM, SIGILL

            .bss
vec:        .res        2
sig:        .res        1
zpkeep:     .res        zpspace
stack:      .res        SIGSTACK
