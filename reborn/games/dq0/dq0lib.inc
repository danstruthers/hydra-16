; ****************************************************************************
; dq0lib.inc - a Dingus Quest Zero task's calls of the game's library (modules/dq0lib: the paged ROM's, ENGINE-
; CRITIQUE.md's §1.4, 2026-11-09), each task's bank.s includes it: the library found as the task starts (MODINFO: the
; module named dq0lib, its bank and page kept), its overlays read in by it (ovl_load: DQ_OVL_LOAD, which calls the
; unpack library: modules/unpack), and each other call an XCALL of its entry (dq0x: some 120 cycles, so nothing called
; in a tight loop).  Its main RAM some 130 bytes, for each task's own ovl_load and num before (and the C library's
; open, strcat ... that they brought in: 88 to 687 bytes of a task's main RAM freed).
;
; Before including it: zeropage.inc; _ovbank (the overlays' banks, OVL_BANKS bytes); OVL_BANKS (the banks the file
; has at most, each a bank of the task's own); OVL_DOT (0, or the banks after which a '.' is written: the UI's
; progress); .define OVL_FILE "/NAME.ovl" (the file, in the folder ovl_load is given: "" for the UI's, whose file
; find_dirs opens).
;   unsigned char __fastcall__ ovl_load (const char* dir)  the file (dir's OVL_FILE) read and unpacked into
;                                                           OVL_BANKS banks of the task's own (_ovbank: the first, the
;                                                           next ...): 0, or not 0 (no library, no file, no banks, the
;                                                           file not as it should be).  First: the library found.
;                                                           The UI's: (unsigned) fd, the file open (closed here)
;   unsigned __fastcall__ num (const char* s)              a number's digits (atoi's, without the C library's ctype)

            .export     _ovl_load, _num
            .import     __SWAP_START__

.scope dql
MODINFO     = $F857                                         ; (hydra.inc's)
XCALL       = $F80C
r0          = $02
r1          = $04
r2          = $06
r3          = $08
r14         = $1E
r15         = $20
ME_BANK     = 0
ME_NAME     = 4
ME_PAGE     = 15
OVL_LOAD    = $30                                           ; (dq0lib's entries: their low bytes, $A0 + its page
NUM         = $33                                           ;   the high)
.endscope

.pushseg
.bss
dq0lib:     .res        2                                   ; The library's bank, and its page's high byte ($A0 on)

.code
; ovl_load (dir): the library found (its directory's entries in the parts' area, none there yet), then its OVL_LOAD
.proc _ovl_load
            pha                                             ; (dir)
            phx
            stz         __SWAP_START__ + 18
find:
            lda         #<__SWAP_START__
            sta         dql::r0
            lda         #>__SWAP_START__
            sta         dql::r0 + 1
            lda         __SWAP_START__ + 18
            inc         __SWAP_START__ + 18
            jsr         dql::MODINFO
            bcs         none                                ; (Past the last: no library, C = 1)
            ldx         #6
name:
            lda         lib,x
            cmp         __SWAP_START__ + dql::ME_NAME,x
            bne         find
            dex
            bpl         name
            lda         __SWAP_START__ + dql::ME_BANK
            sta         dq0lib
            lda         __SWAP_START__ + dql::ME_PAGE
            clc
            adc         #$A0
            sta         dq0lib + 1
            lda         #<file                              ; Its OVL_LOAD: the path made in the parts' area
            sta         dql::r1
            lda         #>file
            sta         dql::r1 + 1
            lda         #<__SWAP_START__
            sta         dql::r2
            lda         #>__SWAP_START__
            sta         dql::r2 + 1
            lda         #<_ovbank
            sta         dql::r3
            lda         #>_ovbank
            sta         dql::r3 + 1
            pla
            sta         dql::r0 + 1
            pla
            sta         dql::r0
            ldx         #OVL_BANKS
            ldy         #OVL_DOT
            lda         #dql::OVL_LOAD
            jsr         dq0x
done:
            lda         #0
            tax
            rol                                             ; (C: 1, it failed)
            rts
none:
            pla
            pla
            bra         done
lib:        .byte       "dq0lib", 0
file:       .byte       OVL_FILE, 0
.endproc

; num (s): its NUM (its low 16 bits)
.proc _num
            sta         dql::r0
            stx         dql::r0 + 1
            lda         #dql::NUM
            ; (fall through)
.endproc

; The library's entry .A (its low byte) called, .X, .Y and r0-r13 its arguments and its results
.proc dq0x
            sta         dql::r15
            lda         dq0lib + 1
            sta         dql::r15 + 1
            lda         dq0lib
            sta         dql::r14
            jmp         dql::XCALL
.endproc
.popseg
