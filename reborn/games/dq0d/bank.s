; ****************************************************************************
; bank.s - (dq0d) the denizens' swapped parts (../dq0/swap.inc): code run in the main RAM's top (dq0d.cfg's SWAP), a
; part at a time, copied there as it's called from the bank its images are in (dq0d.c's ovl_load reads LIBDIR/dq0d.ovl
; into it as the task starts); a call of a part's function goes through its stub here (SWFN).

            .export     _ovbank

            .include    "zeropage.inc"
OVL_BANKS   = 7                     ; (The game's library, ../dq0/dq0lib.inc: ovl_load, num)
OVL_DOT     = 0
.define     OVL_FILE "/dq0d.ovl"
RP_X        = 1                     ; (The record store's calls, ../dq0/rp.h's rp_x)
            .forceimport addeq0sp   ; (popgen's, a bank program's, from this task's runtime: chunk.c's find used it till
                                    ;   the record store's, 2026-11-11)
            .include    "../dq0/dq0lib.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
SWDEPTH     = 8                     ; Swapped parts' calls in calls, at most
SWFAST      = 1                     ; (The parts copied four pages a pass: the swap study's, a fifth of the task's copies)

            .zeropage
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

            .bss
_ovbank:    .res        7           ; The parts' images' banks (dq0d.cfg's OV1 ... OV7)

            .include    "../dq0/swap.inc"

            .include    "parts.inc"         ; (The parts and their stubs, the build's: parts.txt, ../dq0/parts.js)
