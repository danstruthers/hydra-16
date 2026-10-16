; ****************************************************************************
; bank.s - (dq0d) the denizens' swapped parts (../dq0/swap.inc): code run in the main RAM's top (dq0d.cfg's SWAP), a
; part at a time, copied there as it's called from the bank its images are in (dq0d.c's ovl_load reads LIBDIR/dq0d.ovl
; into it as the task starts); a call of a part's function goes through its stub here (SWFN).

            .export     _ovbank

            .include    "zeropage.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 8                     ; Swapped parts' calls in calls, at most

            .bss
_ovbank:    .res        1           ; The parts' images' bank (dq0d.cfg's OV1)
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

            .include    "../dq0/swap.inc"

            SWPART      1, 1                ; The kinds read in, the populations, the placing, the records kept
            SWPART      2, 1                ; The character's ways with a denizen (the UI's sway), and harmed

            SWFN        kinds_load, 1
            SWFN        populate, 1
            SWFN        place, 1
            SWFN        keep, 1
            SWFN        harmed, 2
            SWFN        sway, 2
