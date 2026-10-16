; ****************************************************************************
; bank.s - (dq0w) the world's swapped parts (../dq0/swap.inc): code run in the main RAM's top (dq0w.cfg's SWAP), a
; part at a time, copied there as it's called from the bank its images are in (dq0w.c's ovl_load reads LIBDIR/dq0w.ovl
; into it as the task starts); a call of a part's function goes through its stub here (SWFN).

            .export     _ovbank

            .include    "zeropage.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 8                     ; Swapped parts' calls in calls, at most

            .bss
_ovbank:    .res        1           ; The parts' images' bank (dq0w.cfg's OV1)
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

            .include    "../dq0/swap.inc"

            SWPART      1, 1                ; Digging
            SWPART      2, 1                ; Climbing
            SWPART      3, 1                ; The dungeon begun, the strata below grown

            SWFN        start_dig, 1
            SWFN        dig_on, 1
            SWFN        climb, 2
            SWFN        climb_on, 2
            SWFN        begin, 3
            SWFN        grow_below, 3
