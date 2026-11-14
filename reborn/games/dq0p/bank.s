; ****************************************************************************
; bank.s - (dq0p) the senses' swapped parts (../dq0/swap.inc): code run in the main RAM's top (dq0p.cfg's SWAP), a
; part at a time, copied there as it's called from the bank its image is in (dq0p.c's ovl_load reads LIBDIR/dq0p.ovl
; into them as the task starts: dq0p.cfg's OV1 ...); a call of a part's function goes through its stub here (SWFN).
; A look's own work is in them, once a look; what the senses answer the denizens is the main RAM's.

            .export     _ovbank

            .include    "zeropage.inc"
OVL_BANKS   = 4                     ; (The game's library, ../dq0/dq0lib.inc: ovl_load, num)
OVL_DOT     = 0
.define     OVL_FILE "/dq0p.ovl"
            .include    "../dq0/dq0lib.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 4                     ; Swapped parts' calls in calls, at most
SWFAST      = 1                     ; (The parts copied four pages a pass: two copied each look)

            .zeropage
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

            .bss
_ovbank:    .res        4           ; The parts' images' banks (dq0p.cfg's OV1 ...)

            .include    "../dq0/swap.inc"

            .include    "parts.inc"         ; (The parts and their stubs, the build's: parts.txt, ../dq0/parts.js)
