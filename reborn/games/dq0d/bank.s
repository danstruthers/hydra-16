; ****************************************************************************
; bank.s - (dq0d) the denizens' swapped parts (../dq0/swap.inc): code run in the main RAM's top (dq0d.cfg's SWAP), a
; part at a time, copied there as it's called from the bank its images are in (dq0d.c's ovl_load reads LIBDIR/dq0d.ovl
; into it as the task starts); a call of a part's function goes through its stub here (SWFN).

            .export     _ovbank

            .include    "zeropage.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 8                     ; Swapped parts' calls in calls, at most

            .bss
_ovbank:    .res        3           ; The parts' images' banks (dq0d.cfg's OV1, OV2, OV3)
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

            .include    "../dq0/swap.inc"

            SWPART      1, 1                ; The populations, the placing
            SWPART      2, 1                ; The character's ways with a denizen (the UI's sway), and harmed
            SWPART      3, 2                ; The records paged
            SWPART      4, 2                ; The regions round the character wanted, the records kept; the start
                                            ;   (the kinds read in, popgen loaded);
                                            ;   an actor made, retired, killed, an ally come along,
                                            ;   a cornered one's dart
            SWPART      5, 3                ; The senses' own: a hunter's stalk, a scent followed, one drawn on (slice 4.5's)
            SWPART      6, 3                ; Morale: the field and the bonds, an actor's kin dismayed (2026-10-30)
            SWPART      7, 3                ; Succession (2026-10-30)

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
            SWFN        sway, 2
            SWFN        start, 4
            SWFN        enlist, 4
            SWFN        retire, 4
            SWFN        killed, 4
            SWFN        shoots, 2           ; (A shot at the character: slice 4.7's)
            SWFN        along, 4
            SWFN        dart, 4
            SWFN        dismay, 6
            SWFN        heart, 6
            SWFN        succeed, 7
            SWFN        age, 5               ; (Part 5's since 2026-10-30: part 6's room)
