; ****************************************************************************
; bank.s - (dq0w) the world's swapped parts (../dq0/swap.inc): code run in the main RAM's top (dq0w.cfg's SWAP), a
; part at a time, copied there as it's called from the bank its image is in (dq0w.c's ovl_load reads LIBDIR/dq0w.ovl
; into them as the task starts: dq0w.cfg's OV1 ...); a call of a part's function goes through its stub here (SWFN).

            .export     _ovbank

            .include    "zeropage.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 8                     ; Swapped parts' calls in calls, at most

            .bss
_ovbank:    .res        4           ; The parts' images' banks (dq0w.cfg's OV1 ...: dq0w.c's OVBANKS)
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

            .include    "../dq0/swap.inc"

            SWPART      1, 1                ; Digging
            SWPART      2, 1                ; Climbing
            SWPART      3, 1                ; The dungeon begun, the strata below grown
            SWPART      4, 2                ; The change records' rarer work (store.c), the UI's requests
            SWPART      5, 2                ; Doors (wdoor.c)
            SWPART      6, 3                ; Blows (wblow.c)
            SWPART      7, 3                ; Portcullises and windows (wwin.c)
            SWPART      8, 4                ; Traps and searches (wtrap.c)
            SWPART      9, 4                ; The walks (dq0w.c's go, go_to, step_to)

            SWFN        start_dig, 1
            SWFN        dig_on, 1
            SWFN        climb, 2
            SWFN        climb_on, 2
            SWFN        begin, 3
            SWFN        grow_below, 3
            SWFN        store_in, 4
            SWFN        store_page, 4
            SWFN        store_save, 4
            SWFN        store_begin, 4
            SWFN        answer, 4
            SWFN        stocked, 4
            SWFN        doors, 5
            SWFN        blows, 6
            SWFN        portals, 7
            SWFN        traps, 8
            SWFN        go, 9
            SWFN        go_to, 9
            SWFN        step_to, 9
