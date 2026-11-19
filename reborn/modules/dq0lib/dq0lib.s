; ****************************************************************************
; dq0lib - Dingus Quest Zero's own library (ENGINE-CRITIQUE.md's §1.4 and step 2, 2026-11-09): the code its tasks
; had each a copy of, in the paged ROM once.  A library module (HT_LIBRARY), run in its caller's task through XCALL:
; its jump table at $A030 (games/dq0/dq0lib.inc finds it as a task starts, with its overlays, and calls it).  It has
; no RAM of its own: its arguments and its pointers are the call registers, what it keeps over a call it makes the
; stack's.  Not for a routine called in a tight loop (an XCALL is some 120 cycles): for one that does real work, or
; one called seldom.
;   OVL_LOAD    a task's overlays' file read into banks of its own (each of dq0, dq0w, dq0d, dq0p, dq0m and dq0harn
;               had its ovl_load: the path made, the file opened, the banks had, the unpack library found and
;               called, the file closed)
;   NUM         a number's digits read (atoi's, without the C library's ctype: dq0d's, dq0p's and dq0w's num, the
;               UI's dnum, dq0m's atoi)
;   RP_*        the record store (ENGINE-CRITIQUE.md's §6 and step 7, 2026-11-11: games/dq0/rp.h, each store's records
;               paged by region, the save folder's files of them): a file's name, its versions read and written (each
;               store's copy of vfile.inc before), an entry of a store's table found, and the one to make way (each
;               store's own loops before)
;   AT_FIND     the chunk lookup (ENGINE-CRITIQUE.md's §4 and step 2, 2026-11-14: sq.s, its contract with the tasks
;   CK_*        ../games/dq0/lnzp.inc's): at ()'s long way (each reader's chunk.c's at_find and find), a chunk in the
;               cache or not (the denizens' made, made9, made10, made16, the senses' cached, the world's loops)
;   LN_*        the one line (line.s: the world's and the senses' dq0w/line.inc before)
;   AC_ROW      the world's act table's rows (table.s: games/dq0w/acts.txt, every actor's act its numbers, 2026-11-19)
;   WK_*        the one way-finder and its rule for a square (walk.s: dq0w/walk.inc, compiled into the world and the
;               denizens, before)

.include "hydra.inc"
.include "hyx2.inc"
            .macpack    longbranch

            HYX2_LIBRARY "dq0lib", "HEADER", "ROM"
            .export     rp_find
            .import     at_find, ck_made, ck_at, ln_line, ln_pass, ln_portal, wk_row, wk_seed, wk_search, ac_row

.code
.assert     * = $A030, lderror, "The dq0lib library's entry isn't right after its header"
            jmp         ovl_load                            ; $A030: DQ_OVL_LOAD
            jmp         num                                 ; $A033: DQ_NUM
            jmp         rp_name                             ; $A036: RP_NAME (rp.h's)
            jmp         rp_open                             ; $A039: RP_OPEN
            jmp         rp_make                             ; $A03C: RP_MAKE
            jmp         rp_done                             ; $A03F: RP_DONE
            jmp         rp_find                             ; $A042: RP_FIND
            jmp         rp_oldest                           ; $A045: RP_OLDEST
            jmp         rp_groups                           ; $A048: RP_GROUPS
            jmp         at_find                             ; $A04B: AT_FIND (../games/dq0/lnzp.inc's: sq.s)
            jmp         ck_made                             ; $A04E: CK_MADE
            jmp         ck_at                               ; $A051: CK_AT
            jmp         ln_line                             ; $A054: LN_LINE (line.s)
            jmp         ln_pass                             ; $A057: LN_PASS
            jmp         ln_portal                           ; $A05A: LN_PORTAL
            jmp         wk_row                              ; $A05D: WK_ROW (walk.s)
            jmp         wk_seed                             ; $A060: WK_SEED
            jmp         wk_search                           ; $A063: WK_SEARCH
            jmp         ac_row                              ; $A066: AC_ROW (table.s: the act table, games/dq0w/acts.h)

BANKREG     = $00                                           ; The task's RAM bank register
BUF         = $9FE0                                         ; MODINFO's answer (ME_SIZE bytes; then the entry looked
                                                            ;   at): the first bank's, in the window (unpack fills it
                                                            ;   after)

; OVL_LOAD: a task's overlays' file (its banks, each packed: games/dq0/ovlpack.js) read into n banks of its own, a run
; (BANKS_ALLOC's), and unpacked there (the unpack library's UNPACK_BANKS: modules/unpack); the banks' numbers written
; to the task's table, the first's first.  IN: r0 = the folder (a string), or, its high byte 0, its low byte an fd
; open on the file (the UI's: find_dirs' look for it); r1 = the file's name after the folder ("/dq0d.ovl"); r2 = a
; buffer for the path (the folder's and the name's lengths and one more); r3 = the task's table (n bytes); .X = n;
; .Y = the banks after which a '.' is written (fd 1: the start's progress, the UI's), 0 for none (else under n).
; OUT: C = 0, .A = the first bank; or C = 1 (no file, no banks, no unpack library, the file not as its headers
; say).  The file closed (given or opened); the bank register as it was.  Modifies .A, .X, .Y, r0-r13
F_RUN       = $0101                                         ; (Its frame on the stack, .X = S, the calls changing
F_PAGE      = $0102                                         ;   the call registers: a run's first bank, the unpack
F_UBANK     = $0103                                         ;   library's page (then 0 if it's done, 1 if not) and
F_FIRST     = $0104                                         ;   bank, the first bank, the table, the fd ($FF: none),
F_TAB       = $0105                                         ;   the '.''s banks (0: none, or done), the banks, the
F_FD        = $0107                                         ;   caller's bank register)
F_DOT       = $0108
F_N         = $0109
F_BANK      = $010A
FRAME       = 10

ovl_load:
            lda         BANKREG
            pha                                             ; F_BANK
            phx                                             ; F_N
            phy                                             ; F_DOT
            lda         r0                                  ; F_FD (an fd given)
            pha
            lda         r3 + 1                              ; F_TAB
            pha
            lda         r3
            pha
            pha                                             ; F_FIRST, F_UBANK, F_PAGE (1: failed), F_RUN
            pha
            lda         #1
            pha
            pha
            tsx
            lda         r0 + 1                              ; The file: the path made, the file opened
            beq         @banks
            lda         #$FF                                ; (None yet)
            sta         F_FD,x
            ldy         #0
:
            lda         (r0),y                              ; (The folder ...
            beq         :+
            sta         (r2),y
            iny
            bne         :-
:
            tya                                             ;   then the name, from r4 = r2 + its length)
            clc
            adc         r2
            sta         r4
            lda         r2 + 1
            adc         #0
            sta         r4 + 1
            ldy         #$FF
:
            iny
            lda         (r1),y
            sta         (r4),y
            bne         :-
            lda         r2
            sta         r0
            lda         r2 + 1
            sta         r0 + 1
            lda         #O_READ
            jsr         OPEN
            jcs         @out
            tsx
            sta         F_FD,x
@banks:
            lda         F_N,x                               ; The banks, a run
            jsr         BANKS_ALLOC
            jcs         @out
            tsx
            sta         F_FIRST,x
            sta         F_RUN,x
            lda         F_TAB,x                             ; (The table: the first, the next ...)
            sta         r3
            lda         F_TAB + 1,x
            sta         r3 + 1
            ldy         #0
:
            tya
            clc
            adc         F_FIRST,x
            sta         (r3),y
            iny
            tya
            cmp         F_N,x
            bcc         :-
            lda         F_FIRST,x                           ; The unpack library: its directory entry looked at in
            sta         BANKREG                             ;   the first bank
            stz         BUF + 18
@find:
            lda         #<BUF
            sta         r0
            lda         #>BUF
            sta         r0 + 1
            lda         BUF + 18
            inc         BUF + 18
            jsr         MODINFO
            jcs         @out                                ; (Past the last: none)
            ldy         #6
:
            lda         unpack,y
            cmp         BUF + ME_NAME,y
            bne         @find
            dey
            bpl         :-
            tsx
            lda         BUF + ME_BANK
            sta         F_UBANK,x
            lda         BUF + ME_PAGE
            clc
            adc         #$A0
            sta         F_PAGE,x
@run:
            lda         F_UBANK,x                           ; A run: so many banks (the '.''s, or the rest) from
            sta         r14                                 ;   F_RUN on
            lda         #$30                                ; (UNPACK_BANKS: $A030 in its page)
            sta         r15
            lda         F_PAGE,x
            sta         r15 + 1
            ldy         F_DOT,x
            bne         :+
            ldy         F_N,x
:
            sty         r13
            ldy         F_RUN,x
            lda         F_FD,x
            ldx         r13
            jsr         XCALL
            bcs         @fail
            tsx
            lda         F_DOT,x
            beq         @done
            clc                                             ; The '.': then the rest, after the first run's
            adc         F_RUN,x
            sta         F_RUN,x
            lda         F_N,x
            sec
            sbc         F_DOT,x
            sta         F_N,x
            stz         F_DOT,x
            lda         #'.'                                ; (The '.' on the stack: written from there)
            pha
            tsx
            inx
            stx         r0
            lda         #1
            sta         r0 + 1
            sta         r1
            stz         r1 + 1
            jsr         WRITE                               ; (.A = 1: fd 1)
            pla
            tsx
            bra         @run
@done:
            stz         F_PAGE,x                            ; (0: done)
            bra         @out
@fail:
            tsx
            lda         #1
            sta         F_PAGE,x
@out:
            tsx                                             ; The file closed (if there's one), the bank put back
            lda         F_FD,x
            cmp         #$FF
            beq         :+
            jsr         CLOSE
            tsx
:
            lda         F_BANK,x
            sta         BANKREG
            lda         F_PAGE,x                            ; (Its bit 0: C, failed)
            sta         r13
            ldy         F_FIRST,x
            txa
            clc
            adc         #FRAME
            tax
            txs
            lsr         r13
            tya
            rts

unpack:     .byte       "unpack", 0

; NUM: a number's digits (as many as there are, from the first), its value (32 bits, as they wrap: 2^32 its
; modulus).  IN: r0 = the digits (a string: the first not a digit ends it).  OUT: .A/.X its low 16 bits, r1 its high
; 16 bits; .Y the digits.  Modifies .A, .X, .Y, r1-r5
num:
            stz         r2                                  ; (r2, r3: the value; r4, r5: twice it)
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            ldy         #0
@digit:
            lda         (r0),y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @end
            pha
            asl         r2                                  ; The value ten times: twice it, kept ...
            rol         r2 + 1
            rol         r3
            rol         r3 + 1
            ldx         #3
:
            lda         r2,x
            sta         r4,x
            dex
            bpl         :-
            ldx         #2                                  ;   eight times it ...
:
            asl         r2
            rol         r2 + 1
            rol         r3
            rol         r3 + 1
            dex
            bne         :-
            clc                                             ;   and the two added
            ldx         #0
:
            lda         r2,x
            adc         r4,x
            sta         r2,x
            inx
            txa
            eor         #4
            bne         :-
            pla                                             ; The digit added
            clc
            adc         r2
            sta         r2
            bcc         @next
            inc         r2 + 1
            bne         @next
            inc         r3
            bne         @next
            inc         r3 + 1
@next:
            iny
            bne         @digit
@end:
            lda         r3
            sta         r1
            lda         r3 + 1
            sta         r1 + 1
            lda         r2
            ldx         r2 + 1
            rts

; ---- The record store (games/dq0/rp.h, its C side: ENGINE-CRITIQUE.md's §6, 2026-11-11): what each of the game's
; stores of records paged by region shares.  Each call is given one block of the caller's (rp.h's struct rpf, a
; file's; struct rpt, a table's), in r0; its layout below, rp.h's the same (its checks hold the two together).

; A file's block (struct rpf): a region's file in the save folder (DUNGEON.md's no fixed size), in versions, each
; written whole over a slot that's neither the last save's nor (with three slots) the newest, its header written last
; (struct vfh: VF_MAGIC, the game's generation, its records, its stamp), so a version cut short is none; read, the
; newest valid version: one the last save had (stamped up to the hub's vsaved) or this session's (past its vbase),
; never one a session wrote and ended without its save.  A stamp is past the hub's vlo, the kind's own count from it
; (rp.h's struct rpk, the caller's main RAM's: each kind's own, as each store's copy of vfile.inc had its own)
RF_SLOTS    = 1                                             ; The slots (2 or 3: .dat, .da1, .da2)
RF_PATH     = 2                                             ; (2) The file's name (rp_name's), its last letter its slot
RF_KIND     = 4                                             ; (2) The kind's count (struct rpk: vlast 4, vcount 2)
RF_GEN      = 6                                             ; (2) The game's generation (the world bank's cgen)
RF_HUB      = 8                                             ; (2) The hub's vsaved (then vbase, vlo, vhi: 4 each), in
                                                            ;   the window as it's called
RF_H        = 10                                            ; (10) A version's header: the newest's (open), done's
RF_T        = 20                                            ; (10) A header as it's read
RF_ST       = 30                                            ; (12) Each slot's stamp, if it's valid
RF_SAVE     = 42                                            ; The save's version's slot (3: none)
RF_NEW      = 43                                            ; The newest's (3: none)
RF_FD       = 44                                            ; The newest's fd ($FF none); done's, the fd given
RF_K        = 45                                            ; (A slot)
RF_VFD      = 46                                            ; (An fd)
RF_DIR      = 47                                            ; (2) name's: the save folder (the hub's savedir)
RF_PRE      = 49                                            ; (2) name's: the store's two letters
RF_SN       = 51                                            ; name's: the region's stratum, x and y
RF_RX       = 52
RF_RY       = 53
VF_MAGIC    = $4656                                         ; "VF" (world.h's)

; A table's block (struct rpt): a store's entries, each one's key in its first eight bytes (a mask of them), byte 0
; not 0 if it's used, a stamp (when it was last wanted) at RT_SO
RT_N        = 1                                             ; The entries looked at
RT_SIZE     = 2                                             ; An entry's bytes
RT_KEY      = 3                                             ; The key's bytes: bit i, byte i
RT_AT       = 4                                             ; (2) The table (the window's, or the task's RAM)
RT_SO       = 6                                             ; The stamp's offset
RT_FLAGS    = 7                                             ; RTF_*
RT_STAMP    = 8                                             ; (2) find: the clock (++ into the entry found), or 0
RT_K        = 10                                            ; (8) The key, as an entry has it
RT_FROM     = 18                                            ; find: the first entry looked at
RT_NF       = 19                                            ; find: a free one looked for among the first NF
RT_FREE     = 20                                            ; find's: the first free ($FF none)
RT_R        = 21                                            ; oldest: the box kept about its centre ($FF none)
RT_NSN      = 22                                            ;   its stratum's offset in an entry (one byte),
RT_NX       = 23                                            ;   x's and y's (one byte, or two: RTF_WIDE)
RT_NY       = 24
RT_NV       = 25                                            ; (5) its centre: the stratum, x, y (2 each)
RT_KEEP     = 30                                            ; (2) oldest: the task's routine (.A the entry; .A not
                                                            ;   0, kept), or 0
RTF_FREE    = $01                                           ; oldest: a free one first
RTF_HAND    = $02                                           ; oldest: the key's own kept
RTF_PLAIN   = $04                                           ; oldest: stamps compared plainly (else round, as they wrap)
RTF_WIDE    = $08                                           ; the box's x and y two bytes
RTF_EACH    = $10                                           ; oldest: each free one taken as it's met

; A system call (f), .A, .X, r0 and r1 its; the block's pointer (r12) kept over it, .Y not
.macro SYS f
            ldy         r12
            phy
            ldy         r12 + 1
            phy
            jsr         f
            ply
            sty         r12 + 1
            ply
            sty         r12
.endmacro

; r12 = the block (r0)
enter:
            lda         r0
            sta         r12
            lda         r0 + 1
            sta         r12 + 1
            rts

; r0 = the block's offset .A
bufr0:
            clc
            adc         r12
            sta         r0
            lda         r12 + 1
            adc         #0
            sta         r0 + 1
            rts

; r2 (blk2), r3 (blk3) = the block's offset .A
blk2:
            clc
            adc         r12
            sta         r2
            lda         r12 + 1
            adc         #0
            sta         r2 + 1
            rts
blk3:
            clc
            adc         r12
            sta         r3
            lda         r12 + 1
            adc         #0
            sta         r3 + 1
            rts

; r2 = slot .A's stamp (RF_ST's)
st2:
            asl
            asl
            adc         #RF_ST
            bra         blk2

; r2 = the hub's stamp .A (0 vsaved, 4 vbase, 8 vlo, 12 vhi)
hubp:
            ldy         #RF_HUB
            clc
            adc         (r12),y
            sta         r2
            iny
            lda         (r12),y
            adc         #0
            sta         r2 + 1
            rts

; r3 = the kind's count
kind3:
            ldy         #RF_KIND
            lda         (r12),y
            sta         r3
            iny
            lda         (r12),y
            sta         r3 + 1
            rts

; r0 = the file's name
pathr0:
            ldy         #RF_PATH
            lda         (r12),y
            sta         r0
            iny
            lda         (r12),y
            sta         r0 + 1
            rts

; C = 1: the stamp at r2 under the one at r3 (4 bytes, the low first)
lt4:
            ldy         #3
:
            lda         (r2),y
            cmp         (r3),y
            bne         :+
            dey
            bpl         :-
            clc                                             ; (The same)
            rts
:
            bcs         :+
            sec
            rts
:
            clc
            rts

; The name's last letter made slot .A's; r0 = the name
slotname:
            tax
            jsr         pathr0
            ldy         #0
:
            lda         (r0),y
            beq         :+
            iny
            bne         :-
:
            dey
            lda         slotch,x
            sta         (r0),y
            rts
slotch:     .byte       "t12"

; .A bytes read from RF_VFD to RF_T + .X: C = 0, all of them
readt:
            pha
            sta         r1
            stz         r1 + 1
            txa
            clc
            adc         #RF_T
            jsr         bufr0
            ldy         #RF_VFD
            lda         (r12),y
            SYS         READ
            bcs         @bad
            cpx         #0
            bne         @bad
            sta         r5
            pla
            eor         r5
            cmp         #1                                  ; (C = 0: the same)
            rts
@bad:
            pla
            sec
            rts

; Slot RF_K's version opened, its header read (RF_T): C = 0, it's valid (RF_VFD open past its header); C = 1, it's
; not there, or not valid (not this game's, or a session's that ended without its save: closed)
head:
            ldy         #RF_K
            lda         (r12),y
            jsr         slotname
            lda         #O_READ
            SYS         OPEN
            bcs         @no
            ldy         #RF_VFD
            sta         (r12),y
            lda         #6                                  ; (Its chead: magic, gen, n)
            ldx         #0
            jsr         readt
            bcs         @close
            ldy         #RF_T + 2                           ; Its generation the game's
            lda         (r12),y
            ldy         #RF_GEN
            cmp         (r12),y
            bne         @close
            ldy         #RF_T + 3
            lda         (r12),y
            ldy         #RF_GEN + 1
            cmp         (r12),y
            bne         @close
            ldy         #RF_T                               ; VF_MAGIC
            lda         (r12),y
            cmp         #<VF_MAGIC
            bne         @close
            iny
            lda         (r12),y
            cmp         #>VF_MAGIC
            bne         @close
            lda         #4                                  ; Its stamp: up to vsaved, or past vbase
            ldx         #6
            jsr         readt
            bcs         @close
            lda         #0
            jsr         hubp
            lda         #RF_T + 6
            jsr         blk3
            jsr         lt4
            bcc         @ok
            lda         #4
            jsr         hubp
            jsr         lt4
            bcs         @ok
@close:
            ldy         #RF_VFD
            lda         (r12),y
            SYS         CLOSE
@no:
            sec
            rts
@ok:
            clc
            rts

; The file's slots' versions looked at (each one's stamp in RF_ST; the save's version and the newest: RF_SAVE,
; RF_NEW), the newest left open (RF_FD), its header in RF_H.  OUT: .A = its fd ($FF: none)
scan:
            lda         #3
            ldy         #RF_SAVE
            sta         (r12),y
            iny
            sta         (r12),y                             ; (RF_NEW)
            lda         #$FF
            ldy         #RF_FD
            sta         (r12),y
            lda         #0
            ldy         #RF_K
            sta         (r12),y
@slot:
            ldy         #RF_K
            lda         (r12),y
            ldy         #RF_SLOTS
            cmp         (r12),y
            jcs         @end
            jsr         head
            jcs         @next
            ldy         #RF_K                               ; Its stamp kept
            lda         (r12),y
            asl
            asl
            adc         #RF_ST
            jsr         blk3
            lda         #RF_T + 6
            jsr         blk2
            ldy         #3
:
            lda         (r2),y
            sta         (r3),y
            dey
            bpl         :-
            lda         #0                                  ; The save's: up to vsaved, the newest such
            jsr         hubp
            lda         #RF_T + 6
            jsr         blk3
            jsr         lt4
            bcs         @newest
            ldy         #RF_SAVE
            lda         (r12),y
            cmp         #3
            beq         @save
            jsr         st2
            jsr         lt4
            bcc         @newest
@save:
            ldy         #RF_K
            lda         (r12),y
            ldy         #RF_SAVE
            sta         (r12),y
@newest:
            ldy         #RF_NEW                             ; The newest: kept open, the one before closed
            lda         (r12),y
            cmp         #3
            beq         @new
            jsr         st2
            lda         #RF_T + 6
            jsr         blk3
            jsr         lt4
            bcc         @older
@new:
            ldy         #RF_K
            lda         (r12),y
            ldy         #RF_NEW
            sta         (r12),y
            ldy         #RF_FD
            lda         (r12),y
            cmp         #$FF
            beq         :+
            SYS         CLOSE
:
            ldy         #RF_VFD
            lda         (r12),y
            ldy         #RF_FD
            sta         (r12),y
            lda         #RF_T
            jsr         blk2
            lda         #RF_H
            jsr         blk3
            ldy         #9
:
            lda         (r2),y
            sta         (r3),y
            dey
            bpl         :-
            bra         @next
@older:
            ldy         #RF_VFD
            lda         (r12),y
            SYS         CLOSE
@next:
            ldy         #RF_K
            lda         (r12),y
            inc         a
            sta         (r12),y
            jmp         @slot
@end:
            ldy         #RF_FD
            lda         (r12),y
            rts

; RP_NAME: the save folder's file of a store's region (its two letters, RF_PRE: "rg" ...; preSXXYY.dat, a stratum
; past 15's preSSXXYY.dat), its slot ".dat", in RF_PATH.  OUT: .A/.X 1, or 0 (no folder: RF_DIR "")
rp_name:
            jsr         enter
            ldy         #RF_PATH
            lda         (r12),y
            sta         r3
            iny
            lda         (r12),y
            sta         r3 + 1
            ldy         #RF_DIR
            lda         (r12),y
            sta         r2
            iny
            lda         (r12),y
            sta         r2 + 1
            lda         (r2)
            beq         @none
            ldy         #0
@dir:
            lda         (r2),y
            beq         @sep
            jsr         put
            iny
            bne         @dir
@sep:
            lda         #$2F                                ; ('/')
            jsr         put
            ldy         #RF_PRE
            lda         (r12),y
            jsr         put
            iny
            lda         (r12),y
            jsr         put
            ldy         #RF_SN
            lda         (r12),y
            cmp         #16
            bcc         @one
            jsr         hex2
            bra         @rx
@one:
            jsr         hex1
@rx:
            ldy         #RF_RX
            lda         (r12),y
            jsr         hex2
            ldy         #RF_RY
            lda         (r12),y
            jsr         hex2
            ldy         #0
:
            lda         dat,y
            jsr         put
            iny
            cmp         #0
            bne         :-
            lda         #1
@none:
            ldx         #0
            rts
dat:        .byte       ".dat", 0

hex2:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         hex1
            pla
hex1:
            and         #15
            tax
            lda         hexch,x
            ; (fall through)
put:
            sta         (r3)
            inc         r3
            bne         :+
            inc         r3 + 1
:
            rts
hexch:      .byte       "0123456789abcdef"

; RP_OPEN: the file's newest valid version opened (RF_PATH, RF_SLOTS): .A/.X its fd past its header (RF_H), or -1
rp_open:
            jsr         enter
            jsr         scan
            cmp         #$FF
            bne         :+
            tax
            rts
:
            ldx         #0
            rts

; RP_MAKE: a new version of the file begun, over a slot that's neither the save's version nor the newest (else, two
; slots, over the newest), its stamp the kind's next: .A/.X its fd, past the room for its header, or -1 (the hub's
; vhi 0: the game not begun; or not made)
rp_make:
            jsr         enter
            jsr         scan
            cmp         #$FF
            beq         :+
            SYS         CLOSE
:
            ldx         #0                                  ; The slot
@k:
            txa
            ldy         #RF_SAVE
            cmp         (r12),y
            beq         @inc
            ldy         #RF_NEW
            cmp         (r12),y
            bne         @got
@inc:
            inx
            bra         @k
@got:
            ldy         #RF_SLOTS
            cmp         (r12),y
            bcc         @slot
            ldy         #RF_SAVE                            ; (None free: !save's)
            lda         (r12),y
            beq         :+
            lda         #0
            bra         @slot
:
            lda         #1
@slot:
            ldy         #RF_K
            sta         (r12),y
            lda         #12                                 ; vhi 0: none
            jsr         hubp
            ldy         #3
            lda         (r2),y
            dey
            ora         (r2),y
            dey
            ora         (r2),y
            dey
            ora         (r2),y
            jeq         @fail
            lda         #8                                  ; The kind's count: from vlo, as it's moved on
            jsr         hubp
            jsr         kind3
            ldy         #3
:
            lda         (r2),y
            cmp         (r3),y
            bne         @moved
            dey
            bpl         :-
            bra         @count
@moved:
            ldy         #3
:
            lda         (r2),y
            sta         (r3),y
            dey
            bpl         :-
            lda         #0
            ldy         #4
            sta         (r3),y
            iny
            sta         (r3),y
@count:
            ldy         #4
            lda         (r3),y
            inc         a
            sta         (r3),y
            bne         :+
            iny
            lda         (r3),y
            inc         a
            sta         (r3),y
:
            ldy         #RF_K                               ; Opened (made, if it isn't there)
            lda         (r12),y
            jsr         slotname
            lda         #O_WRITE | O_TRUNC
            SYS         OPEN
            bcc         @open
            cmp         #E_NOENT
            bne         @fail
            jsr         pathr0
            lda         #O_WRITE | O_TRUNC
            ldx         #0
            SYS         CREATE
            bcs         @fail
@open:
            ldy         #RF_VFD
            sta         (r12),y
            lda         #0                                  ; (No version till its header's written: done's)
            ldy         #RF_H
            sta         (r12),y
            iny
            sta         (r12),y
            jsr         hwrite
            ldy         #RF_VFD
            lda         (r12),y
            ldx         #0
            rts
@fail:
            lda         #$FF
            tax
            rts

; RF_H written to RF_VFD
hwrite:
            lda         #RF_H
            jsr         bufr0
            lda         #10
            sta         r1
            stz         r1 + 1
            ldy         #RF_VFD
            lda         (r12),y
            SYS         WRITE
            rts

; RP_DONE: the version's header written (RF_FD, RF_H's n: its records; its stamp the kind's last), it closed
rp_done:
            jsr         enter
            ldy         #RF_H
            lda         #<VF_MAGIC
            sta         (r12),y
            iny
            lda         #>VF_MAGIC
            sta         (r12),y
            ldy         #RF_GEN
            lda         (r12),y
            ldy         #RF_H + 2
            sta         (r12),y
            ldy         #RF_GEN + 1
            lda         (r12),y
            ldy         #RF_H + 3
            sta         (r12),y
            jsr         kind3                               ; Its stamp: vlast + vcount
            clc
            ldy         #0
            lda         (r3),y
            ldy         #4
            adc         (r3),y
            ldy         #RF_H + 6
            sta         (r12),y
            ldy         #1
            lda         (r3),y
            ldy         #5
            adc         (r3),y
            ldy         #RF_H + 7
            sta         (r12),y
            ldy         #2
            lda         (r3),y
            adc         #0
            ldy         #RF_H + 8
            sta         (r12),y
            ldy         #3
            lda         (r3),y
            adc         #0
            ldy         #RF_H + 9
            sta         (r12),y
            ldy         #RF_FD
            lda         (r12),y
            ldy         #RF_VFD
            sta         (r12),y
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #0                                  ; (From the start)
            SYS         SEEK
            jsr         hwrite
            ldy         #RF_VFD
            lda         (r12),y
            SYS         CLOSE
            rts

; r2 = the table, r3 (its low byte) = 0 (the entry), r4 = $FF (found), r5 = $FF (free)
tstart:
            ldy         #RT_AT
            lda         (r0),y
            sta         r2
            iny
            lda         (r0),y
            sta         r2 + 1
            stz         r3
            lda         #$FF
            sta         r4
            sta         r5
            rts

; The next entry
tnext:
            ldy         #RT_SIZE
            lda         (r0),y
            clc
            adc         r2
            sta         r2
            bcc         :+
            inc         r2 + 1
:
            inc         r3
            rts

; Z = 1: the entry (r2) has the key
match:
            ldy         #RT_KEY
            lda         (r0),y
            sta         r7
            ldx         #0
@byte:
            lsr         r7
            bcc         @skip
            txa
            tay
            lda         (r2),y
            sta         r8
            txa
            clc
            adc         #RT_K
            tay
            lda         (r0),y
            cmp         r8
            bne         @no
@skip:
            inx
            lda         r7
            bne         @byte
@no:
            rts                                             ; (Z = 1 from lda r7 = 0: all of it the same)

; RP_FIND: the entry with the key (from RT_FROM on), its stamp made the clock's next (RT_STAMP, if it's given); and
; the first free one among the first RT_NF (RT_FREE: $FF none).  OUT: .A = the entry, or $FF.  (The chunks' cache's
; lookup, each time a task's square is in another chunk: the block's numbers and the key's bytes, as offset and value
; pairs, in the call registers first, so an entry's some 60 cycles)
rp_find:
            jsr         tstart
            ldy         #RT_N                               ; r1: the entries, the first looked at; r4 + 1: an
            lda         (r0),y                              ;   entry's bytes; r5 + 1: NF
            sta         r1
            ldy         #RT_FROM
            lda         (r0),y
            sta         r1 + 1
            ldy         #RT_SIZE
            lda         (r0),y
            sta         r4 + 1
            ldy         #RT_NF
            lda         (r0),y
            sta         r5 + 1
            ldy         #RT_KEY                             ; The key: offsets r6-r9, values r10-r13, r3 + 1 of them
            lda         (r0),y
            sta         r3 + 1
            ldx         #0
            ldy         #0
@pair:
            lsr         r3 + 1
            bcc         @skip
            tya
            sta         r6,x
            phy
            clc
            adc         #RT_K
            tay
            lda         (r0),y
            sta         r10,x
            ply
            inx
@skip:
            iny
            lda         r3 + 1
            bne         @pair
            stx         r3 + 1
@loop:
            lda         r3
            cmp         r1
            jcs         @done
            cmp         r1 + 1
            bcc         @next
            lda         (r2)
            bne         @used
            lda         r5                                  ; Free: the first among the first NF
            cmp         #$FF
            bne         @next
            lda         r3
            cmp         r5 + 1
            bcs         @next
            sta         r5
            lda         r4
            cmp         #$FF
            bne         @done
            bra         @next
@used:
            lda         r4
            cmp         #$FF
            bne         @next
            ldx         r3 + 1
            beq         @hit
            ldx         #0
@match:
            ldy         r6,x
            lda         (r2),y
            cmp         r10,x
            bne         @next
            inx
            cpx         r3 + 1
            bne         @match
@hit:
            lda         r3
            sta         r4
            ldy         #RT_STAMP                           ; Its stamp: ++ the clock
            lda         (r0),y
            sta         r6
            iny
            lda         (r0),y
            sta         r6 + 1
            ora         r6
            beq         @stamped
            lda         (r6)
            inc         a
            sta         (r6)
            bne         :+
            ldy         #1
            lda         (r6),y
            inc         a
            sta         (r6),y
:
            ldy         #RT_SO
            lda         (r0),y
            tay
            lda         (r6)
            sta         (r2),y
            iny
            phy
            ldy         #1
            lda         (r6),y
            ply
            sta         (r2),y
@stamped:
            lda         r5                                  ; (Done if a free one's found, or none's to be had past it)
            cmp         #$FF
            bne         @done
            lda         r3
            inc         a
            cmp         r5 + 1
            bcs         @done
@next:
            lda         r2
            clc
            adc         r4 + 1
            sta         r2
            bcc         :+
            inc         r2 + 1
:
            inc         r3
            jmp         @loop
@done:
            lda         r5
            ldy         #RT_FREE
            sta         (r0),y
            lda         r4
            rts

; RP_OLDEST: the entry least lately wanted (its stamp the oldest: round, as they wrap, or plainly, RTF_PLAIN), but
; those kept: the key's own (RTF_HAND), those in the box about RT_NV (RT_R each way, on its stratum), those the task's
; routine keeps (RT_KEEP); a free one first (RTF_FREE), or each free one taken as it's met, its stamp the one the
; entries after it are held to (RTF_EACH: the last free one, unless one after it is older than its stamp: the plans'
; pages' own way).  OUT: .A = the entry, or $FF
rp_oldest:
            jsr         tstart
@loop:
            lda         r3
            ldy         #RT_N
            cmp         (r0),y
            jcs         @done
            lda         (r2)
            bne         @used
            ldy         #RT_FLAGS
            lda         (r0),y
            and         #RTF_FREE
            beq         :+
            lda         r3
            rts
:
            lda         (r0),y
            and         #RTF_EACH
            beq         @next
            ldy         #RT_SO                              ; (Taken: its stamp the best's)
            lda         (r0),y
            tay
            lda         (r2),y
            sta         r6
            iny
            lda         (r2),y
            sta         r6 + 1
            bra         @take
@used:
            ldy         #RT_FLAGS
            lda         (r0),y
            and         #RTF_HAND
            beq         :+
            jsr         match
            beq         @next
:
            ldy         #RT_R
            lda         (r0),y
            cmp         #$FF
            beq         :+
            jsr         near
            bcs         @next
:
            ldy         #RT_SO                              ; Its stamp (r6): older than the best's (r9)?
            lda         (r0),y
            tay
            lda         (r2),y
            sta         r6
            iny
            lda         (r2),y
            sta         r6 + 1
            lda         r4
            cmp         #$FF
            beq         @cand
            ldy         #RT_FLAGS
            lda         (r0),y
            and         #RTF_PLAIN
            beq         @round
            lda         r6
            cmp         r9
            lda         r6 + 1
            sbc         r9 + 1
            bcs         @next
            bra         @cand
@round:
            lda         r6                                  ; (Its stamp less the best's past $8000)
            sec
            sbc         r9
            sta         r10
            lda         r6 + 1
            sbc         r9 + 1
            cmp         #$80
            bcc         @next
            bne         @cand
            lda         r10
            beq         @next
@cand:
            ldy         #RT_KEEP + 1
            lda         (r0),y
            beq         @take
            jsr         keep
            bcs         @next
@take:
            lda         r3
            sta         r4
            lda         r6
            sta         r9
            lda         r6 + 1
            sta         r9 + 1
@next:
            jsr         tnext
            jmp         @loop
@done:
            lda         r4
            rts

; C = 1: the entry (r2) on the box's stratum, its x and y within RT_R of its centre's
near:
            ldy         #RT_NSN
            lda         (r0),y
            tay
            lda         (r2),y
            ldy         #RT_NV
            cmp         (r0),y
            bne         @far
            ldy         #RT_NX
            lda         (r0),y
            ldx         #RT_NV + 1
            jsr         axis
            bcc         @far
            ldy         #RT_NY
            lda         (r0),y
            ldx         #RT_NV + 3
            jmp         axis
@far:
            clc
            rts

; .A = an entry's offset of x or y, .X = the block's of the centre's: C = 1, (its - the centre's + R) up to 2R (two
; bytes, or one: RTF_WIDE)
axis:
            tay
            lda         (r2),y
            sta         r7
            iny
            lda         (r2),y
            sta         r7 + 1
            txa
            tay
            lda         r7
            sec
            sbc         (r0),y
            sta         r7
            iny
            lda         r7 + 1
            sbc         (r0),y
            sta         r7 + 1
            ldy         #RT_R
            lda         r7
            clc
            adc         (r0),y
            sta         r7
            bcc         :+
            inc         r7 + 1
:
            ldy         #RT_FLAGS
            lda         (r0),y
            and         #RTF_WIDE
            bne         :+
            stz         r7 + 1
:
            ldy         #RT_R
            lda         (r0),y
            asl
            sta         r8
            lda         #0
            rol
            sta         r8 + 1
            lda         r8
            cmp         r7
            lda         r8 + 1
            sbc         r7 + 1
            rts

; The task's routine (RT_KEEP) asked of the entry (.A, r3's): C = 1, kept (its .A not 0).  r0-r9 kept over it on the
; stack (its zero page is cc65's, from $22: the call registers are kept only to be sure)
keep:
            ldy         #RT_KEEP
            lda         (r0),y
            sta         r11
            iny
            lda         (r0),y
            sta         r11 + 1
            ldx         #0
:
            lda         r0,x
            pha
            inx
            cpx         #r10 - r0
            bcc         :-
            lda         r3
            jsr         @call
            cmp         #1
            ldx         #r10 - r0 - 1
:
            ply
            sty         r0,x
            dex
            bpl         :-
            rts
@call:
            jmp         (r11)

; A region's file of chunks' groups (struct rpg: the things', the traces': each chunk's records a group, its header
; first, its place in the region at byte 0, its count at RG_NO)
RG_IN       = 1                                             ; The file read (an fd)
RG_OUT      = 2                                             ; The version written ($FF: none)
RG_AT       = 3                                             ; The chunk at hand's place
RG_GSIZE    = 4                                             ; A group's header's bytes (up to 8)
RG_NO       = 5                                             ; Its count's offset in it
RG_RSIZE    = 6                                             ; A record's bytes (up to 255)
RG_BUF      = 7                                             ; (2) A record's room (the task's RAM)
RG_ALL      = 9                                             ; (2) The records copied, added to
RG_G        = 11                                            ; (8) A group's header, as it's read

; RP_GROUPS: the groups in the file (RG_IN) read past, each but the chunk at hand's copied to RG_OUT (their counts
; added to RG_ALL), or (RG_OUT $FF) to the chunk at hand's: its header in RG_G.  OUT: .A/.X 1, the chunk at hand's
; found (RG_OUT $FF); 0, the file's end
rp_groups:
            jsr         enter
@group:
            lda         #RG_G                               ; A group's header
            jsr         bufr0
            ldy         #RG_GSIZE
            lda         (r12),y
            sta         r1
            stz         r1 + 1
            pha
            ldy         #RG_IN
            lda         (r12),y
            SYS         READ
            ply
            jcs         @end
            cpx         #0
            jne         @end
            sty         r5
            cmp         r5
            jne         @end
            ldy         #RG_G                               ; The chunk at hand's?
            lda         (r12),y
            ldy         #RG_AT
            cmp         (r12),y
            php
            ldy         #RG_OUT
            lda         (r12),y
            cmp         #$FF
            bne         @copy
            plp
            bne         @skip
            lda         #1                                  ; (Found)
            ldx         #0
            rts
@copy:
            plp
            bne         @out
@skip:
            ldy         #RG_NO                              ; Its records passed: its count times a record's bytes
            lda         (r12),y
            clc
            adc         #RG_G
            tay
            lda         (r12),y
            sta         r2
            iny
            lda         (r12),y
            sta         r2 + 1
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldy         #RG_RSIZE
            lda         (r12),y
            sta         r3
            ldx         #8
@mul:
            asl         r0
            rol         r0 + 1
            rol         r1
            asl         r3
            bcc         :+
            lda         r0
            clc
            adc         r2
            sta         r0
            lda         r0 + 1
            adc         r2 + 1
            sta         r0 + 1
            bcc         :+
            inc         r1
:
            dex
            bne         @mul
            ldy         #RG_IN
            lda         (r12),y
            ldx         #1                                  ; (From where it is)
            SYS         SEEK
            jmp         @group
@out:
            lda         #RG_G                               ; Copied: its header, then its records
            jsr         bufr0
            ldy         #RG_GSIZE
            lda         (r12),y
            sta         r1
            stz         r1 + 1
            ldy         #RG_OUT
            lda         (r12),y
            SYS         WRITE
            ldy         #RG_NO
            lda         (r12),y
            clc
            adc         #RG_G
            tay
            lda         (r12),y                             ; (Its count: r2, added to RG_ALL)
            sta         r2
            iny
            lda         (r12),y
            sta         r2 + 1
            ldy         #RG_ALL
            lda         (r12),y
            clc
            adc         r2
            sta         (r12),y
            iny
            lda         (r12),y
            adc         r2 + 1
            sta         (r12),y
@rec:
            lda         r2
            ora         r2 + 1
            jeq         @group
            lda         r2                                  ; (Kept over the calls: on the stack)
            pha
            lda         r2 + 1
            pha
            ldy         #RG_IN
            jsr         @one
            ldy         #RG_OUT
            jsr         @one
            pla
            sta         r2 + 1
            pla
            sta         r2
            lda         r2
            bne         :+
            dec         r2 + 1
:
            dec         r2
            bra         @rec
@one:
            phy                                             ; A record read (.Y RG_IN) or written (RG_OUT)
            ldy         #RG_BUF
            lda         (r12),y
            sta         r0
            iny
            lda         (r12),y
            sta         r0 + 1
            ldy         #RG_RSIZE
            lda         (r12),y
            sta         r1
            stz         r1 + 1
            ply
            lda         (r12),y
            cpy         #RG_IN
            bne         :+
            SYS         READ
            rts
:
            SYS         WRITE
            rts
@end:
            lda         #0
            tax
            rts
