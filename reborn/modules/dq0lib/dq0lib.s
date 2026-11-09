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

.include "hydra.inc"
.include "hyx2.inc"
            .macpack    longbranch

            HYX2_LIBRARY "dq0lib", "HEADER", "ROM"
.code
.assert     * = $A030, lderror, "The dq0lib library's entry isn't right after its header"
            jmp         ovl_load                            ; $A030: DQ_OVL_LOAD
            jmp         num                                 ; $A033: DQ_NUM

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
