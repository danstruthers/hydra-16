; ****************************************************************************
; bank.s - (dq0p) the senses' swapped parts (../dq0/swap.inc): code run in the main RAM's top (dq0p.cfg's SWAP), a
; part at a time, copied there as it's called from the bank its image is in (dq0p.c's ovl_load reads LIBDIR/dq0p.ovl
; into them as the task starts: dq0p.cfg's OV1 ...); a call of a part's function goes through its stub here (SWFN).
; A look's own work is in them, once a look; what the senses answer the denizens is the main RAM's.

            .export     _ovbank

            .include    "zeropage.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 4                     ; Swapped parts' calls in calls, at most
SWFAST      = 1                     ; (The parts copied four pages a pass: two copied each look)

            .bss
_ovbank:    .res        3           ; The parts' images' banks (dq0p.cfg's OV1 ...)
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

            .include    "../dq0/swap.inc"

            SWPART      1, 1                ; A look's squares seen (view.c, cast.s), its walk felt, its lamps (things.c)
            SWPART      2, 2                ; What the character perceives made (sight.c)
            SWPART      3, 3                ; A look one way's own record (way.c)
            SWPART      4, 3                ; What the character keeps of a look (view.c's kept: slice 4.2's)

            SWFN        view, 1
            SWFN        felt, 2
            SWFN        things_lamps, 1
            SWFN        lamp_on, 1
            SWFN        perceive, 2
            SWFN        way_seen, 3
            SWFN        kept, 4
