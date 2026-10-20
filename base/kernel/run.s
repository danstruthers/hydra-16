; ****************************************************************************
; run.s - bank programs (reborn/docs/design/plans/RUN.md): a task's code cache, the loader, RUN and a run's end,
; SPAWN_RUN, a bank program in a task of its own.  The calls' entries and the run itself are on page 0 (a program runs
; with page 0 at $E000, where the jump table is); the cache and the loader on BIOS ROM page 5.
;
; A bank program's file is a HYX2 header (HF_BANK) and an o65 module after it, as ld65 links it (sdk/asm/bank.cfg):
; its text (its own HYX2 header first: its name, its main), data and BSS one after another from $8000, its zero page
; from 0, then its relocations.
;   The code cache: RAM banks of the task's own (BANKS_ALLOC's: TK_RCACHE the first, each one's RB_NEXT the next),
; RC_BANKS at most (or TK_RMAX), forgotten as the task starts a program (K_MEM_START: an EXEC's, a SPAWN's), so freed
; with the rest.  A bank holds modules from RB_MODS to RB_TOP, one above another; its last 256 bytes are its
; directory (RB_DIR: RB_NEXT, then RB_ENTS entries, RE_*), and the 256 below them the loader's scratch (RS_*).  A
; module goes in the lowest gap it fits, in the first bank with one; else in a new bank; else the module used least
; lately (and not running) is given up, and the next, till it fits.  A program is known by its file's identity
; (RE_ID: a hash of STAT's qid, version, length, time and device), so a changed file, or another file under the
; name after a bind, is loaded again.
;   RUN: the program found (or loaded), its run frame (in its module, after its BSS: RFR_*) made of what's the
; caller's (its fds, note handler, break, the zero page where the program's goes, its banks at $8000 and $A000, the
; stack), the program's fds the caller's (or the map's), its zero page cleared, then its main with r0 its arguments
; (the task keeps its name: the shell's, as a window's chrome shows it).  Its return, or EXITS (a note's default too: notes.s), is the run's end:
; the program's fds closed, the caller's put back with the rest, the note that ended it (but a kill) posted again
; for the caller's own handler, and RUN returns its code and message.

.include "kdefs.inc"
.macpack    longbranch

; The loader's scratch, in the bank it works in (RB_SCRATCH: 256 bytes)
RS_HDR      = RB_SCRATCH                ; (26) A module's o65 header ...
RS_TEXT     = RB_SCRATCH + 26           ; (2)   and its text's place in its file (past the header's options)
RS_HSIZE    = 28                        ;   (The two, together)
RS_DELTA    = RB_SCRATCH + 28           ; (2) Where the module is, less where it was linked ($8000)
RS_ZPAT     = RB_SCRATCH + 30           ; Where its zero page is (and RS_ZPD, less where it was linked)
RS_KIND     = RB_SCRATCH + 31           ; RF_PROG, or 0 (a child)
RS_VAL      = RB_SCRATCH + 32           ; (2) A relocation's value
RS_TYPE     = RB_SCRATCH + 34           ;   and its type
RS_END      = RB_SCRATCH + 35           ; (2) The image's end (its BSS's): its table's place
RS_DST      = RB_SCRATCH + 37           ; (2) rl_readx's: where the bytes go ...
RS_CNT      = RB_SCRATCH + 39           ; (2)   and how many are still to come
RS_BP       = RB_SCRATCH + 41           ; (2) The read buffer (rl_byte's): where it is ...
RS_BSZ      = RB_SCRATCH + 43           ; (2)   and its size
RS_ZPD      = RB_SCRATCH + 45           ; Its zero page's move
RS_LIM      = RB_SCRATCH + 46           ; (2) Its room's end (its entry's RE_END, as it's placed)
RS_NIMP     = RB_SCRATCH + 48           ; A child's imports: how many ...
RS_NAMES    = RB_SCRATCH + 49           ; (2)   their names (past its BSS) ...
RS_IMPA     = RB_SCRATCH + 51           ; (2)   and their addresses
RS_XB       = RB_SCRATCH + 53           ; (2) The exports' file's reader (rx_byte's): its buffer ...
RS_ENAME    = RB_SCRATCH + 55           ; (2)   an export's name read ...
RS_XP       = RB_SCRATCH + 57           ; (2)   its next byte ...
RS_XC       = RB_SCRATCH + 59           ;   the bytes left in it ...
RS_XFD      = RB_SCRATCH + 60           ;   and the file
RS_SAVP     = RB_SCRATCH + 61           ; (2) rl_byte's place (K_PTR2, K_CNT) kept meanwhile
RS_SAVC     = RB_SCRATCH + 63           ; (2)
RS_MISS     = RB_SCRATCH + 65           ; (2) A missing import's name
RS_TEND     = RB_SCRATCH + 67           ; (2) A child's table's end
RS_BUF      = RB_SCRATCH + 80           ; (176) The scratch's own buffer (a file's first bytes, a stat record)
RS_BUFLEN   = 176
RX_BUFLEN   = 64                        ; The exports' reader's buffer, and a name in it at most
RX_NAMEMAX  = 31
CHILD_ROOM  = 1024                      ; A child's room for its imports' names and addresses, then its exports' table

O65_MODE    = 6                         ; The o65 header: its mode (2) ...
O65_TBASE   = 8                         ;   text's base and length ...
O65_TLEN    = 10
O65_DBASE   = 12                        ;   data's ...
O65_DLEN    = 14
O65_BBASE   = 16                        ;   BSS's ...
O65_BLEN    = 18
O65_ZBASE   = 20                        ;   the zero page's ...
O65_ZLEN    = 22
O65_HSIZE   = 26
LINKED      = $8000                     ; Where a module's text is linked

; ****************************************************************************
.segment "KCODE"

; RUN: the bank program at r0 run in this task, as a subroutine, and waited for.  IN: r0 = its path; r1 = its
; arguments (as SPAWN's), or 0; .A = flags (SPAWN_FDMAP, with r2 = an fd map as SPAWN's; RUN_LEND).  OUT: C = 0, .A
; = its exit code, r0 = its exit message (in TA_SCRATCH); or C = 1, .A = the error (E_NOEXEC: not a bank program
; that runs in place)
K_RUN:
            jsr         run_core
            jmp         K_NOTE_CHECK                        ; (A note pending: the caller's, taken on the way out)

; RUN's work: the program ready (page 5), the stack it returns with kept in its frame, its main called; then the
; run's end.  OUT: as RUN's
run_core:
            FARCALL     K_RUN_PREP
            bcs         run_rts
            tsx                                             ; (run_core's caller's: K_RUN_EXIT goes back there)
            txa
            ldy         #RFR_S
            sta         (TK_RFRAME),Y
            jsr         run_main
            lda         #0                                  ; (Main returned: code 0, no message)
            stz         r0
            stz         r0 + 1
; A run's end (EXITS's, in a program running in place).  IN: .A = the code; r0 = the message, or 0
K_RUN_EXIT:
            pha
            ldy         #0                                  ; Its message (EXITS's way, on page 0: a note's name is
            lda         r0                                  ;   here)
            ora         r0 + 1
            beq         @msgend
@msg:
            lda         (r0),Y
            beq         @msgend
            sta         TA_SCRATCH,Y
            iny
            cpy         #EXIT_MSG_MAX
            bne         @msg
@msgend:
            lda         #0
            sta         TA_SCRATCH,Y
            pla
            FARCALL     K_RUN_END                           ; .A = the code, .X = the stack
            txs
            ldx         #<TA_SCRATCH
            stx         r0
            ldx         #>TA_SCRATCH
            stx         r0 + 1
            clc
run_rts:
            rts

run_main:
            jmp         (R_ENTRY)

; SPAWN (the jump table's).  With SPAWN_RUN, a bank program that runs in place runs here (run_core), its status
; this task's exit record (WAIT's), and this task is the one given back; anything else (a program that isn't one,
; or can't run here now) is SPAWN's (load.s)
K_SPAWN0:
            bit         #SPAWN_RUN
            beq         @spawn
            bit         #SPAWN_NEWGROUP | SPAWN_NEWNS | SPAWN_NOENV | SPAWN_STOPPED
            bne         @plain                              ; (What only a task of its own can have)
            pha
            jsr         run_core
            bcs         @not
            KCALL       K_RUN_RECORD                        ; (Its code, its message in TA_SCRATCH)
            pla
            jsr         K_GETPID
            jmp         K_NOTE_CHECK

@not:
            cmp         #E_NOENT                            ; (No such file: SPAWN's answer too)
            bne         :+
            plx
            sec
            jmp         K_NOTE_CHECK

:
            pla
@plain:
            and         #<~(SPAWN_RUN | RUN_LEND)
@spawn:
            FARCALL     K_SPAWN
            jmp         K_NOTE_CHECK

; A bank program's start in a task of its own (its TA_ENTRY: K_LOAD's): it's loaded into a bank of the task's (its
; file at LOAD_FD), then its main, with r0 its arguments; one that can't be loaded ends, its error its code
K_BANK_START:
            FARCALL     K_BANK_TASKLOAD
            bcs         @failed
            lda         #<TA_ARGS
            sta         r0
            lda         #>TA_ARGS
            sta         r0 + 1
            jmp         (R_ENTRY)

@failed:
            stz         r0
            stz         r0 + 1
            jmp         K_EXITS

; ****************************************************************************
.segment "KCODE_P5"

; RUN's first half: the program found in the cache (or loaded), its frame made, the task its.  IN: as RUN's.  OUT:
; C = 0: its bank in the window, R_ENTRY its main, r0 its arguments, TK_RBANK and TK_RFRAME its; or C = 1, .A = the
; error, the window and r0-r2 as they came (for a SPAWN after)
K_RUN_PREP:
            sta         L_FLAGS
            lda         r0
            sta         L_PATH
            lda         r0 + 1
            sta         L_PATH + 1
            lda         r1
            sta         L_ARGS
            lda         r1 + 1
            sta         L_ARGS + 1
            lda         r2
            sta         R_MAP
            lda         r2 + 1
            sta         R_MAP + 1
            lda         RAM_BANK
            sta         R_RAMB
            lda         L_PATH + 1                          ; (The path, the arguments and the map not in the
            jsr         rc_window                           ;   window: the cache's while it's looked through)
            bcs         @fault
            lda         L_ARGS + 1
            jsr         rc_window
            bcs         @fault
            lda         L_FLAGS
            and         #SPAWN_FDMAP
            beq         :+
            lda         R_MAP + 1
            jsr         rc_window
            bcs         @fault
:
            jsr         rc_stat                             ; R_ID: the file's identity
            bcs         @fail
            lda         #RF_PROG
            ldx         #PROG_ZP
            jsr         rc_find
            bcc         @have
            jsr         rl_prog                             ; (Not in the cache: loaded)
            bcs         @fail
@have:
            jsr         re_at
            ldy         #RE_FLAGS
            lda         (K_PTR),Y
            and         #RF_BUSY
            beq         :+
            lda         #E_BUSY                             ; (Running already: one that runs itself)
            bra         @fail

:
            jsr         run_frame
            clc
            rts

@fault:
            lda         #E_FAULT
@fail:
            pha
            lda         R_RAMB
            sta         RAM_BANK
            lda         L_PATH
            sta         r0
            lda         L_PATH + 1
            sta         r0 + 1
            lda         L_ARGS
            sta         r1
            lda         L_ARGS + 1
            sta         r1 + 1
            lda         R_MAP
            sta         r2
            lda         R_MAP + 1
            sta         r2 + 1
            pla
            sec
            rts

; C = 1 if .A, an address's high byte, is the window's ($80-$9F)
rc_window:
            cmp         #$80
            bcc         @no
            cmp         #$A0
            bcs         @no
            sec
            rts

@no:
            clc
            rts

; The run's frame, for the module R_BANK and R_ENT (its bank in the window): what's the caller's kept in it, and
; the task the program's: its fds, no note handler, its break, its zero page cleared; R_ENTRY its main, r0 its
; arguments
run_frame:
            jsr         re_at
            ldy         #RE_TABLE                           ; K_PTR2: the frame
            lda         (K_PTR),Y
            sta         K_PTR2
            iny
            lda         (K_PTR),Y
            sta         K_PTR2 + 1
            ldy         #RE_FLAGS                           ; (Running, and used now)
            lda         (K_PTR),Y
            ora         #RF_BUSY
            sta         (K_PTR),Y
            jsr         re_stamp
            ldx         #1                                  ; Its data as it was loaded, its BSS cleared: a new
            jsr         rf_data                             ;   run's, as a new load's
            jsr         re_at
            ldy         #RFR_PBANK                          ; The run it's in (if any) ...
            lda         TK_RBANK
            sta         (K_PTR2),Y
            ldy         #RFR_PFRAME
            lda         TK_RFRAME
            sta         (K_PTR2),Y
            iny
            lda         TK_RFRAME + 1
            sta         (K_PTR2),Y
            ldy         #RFR_RAMB                           ; ... the caller's banks ...
            lda         R_RAMB
            sta         (K_PTR2),Y
            iny
            lda         ROM_BANK
            sta         (K_PTR2),Y
            ldy         #RFR_NOTIFY                         ; ... note handler, break ...
            lda         TA_NOTIFY
            sta         (K_PTR2),Y
            iny
            lda         TA_NOTIFY + 1
            sta         (K_PTR2),Y
            iny
            lda         TA_BRK
            sta         (K_PTR2),Y
            iny
            lda         TA_BRK + 1
            sta         (K_PTR2),Y
            iny
            lda         TA_BRKMIN
            sta         (K_PTR2),Y
            iny
            lda         TA_BRKMIN + 1
            sta         (K_PTR2),Y
            iny
            lda         TK_PREEMPT
            sta         (K_PTR2),Y
            iny
            lda         TK_INNOTE
            sta         (K_PTR2),Y
            iny
            lda         L_FLAGS
            sta         (K_PTR2),Y
            iny
            lda         R_ENT
            sta         (K_PTR2),Y
            iny
            lda         #0                                  ; (RFR_NIL: no arguments)
            sta         (K_PTR2),Y
            ldx         #0                                  ; ... fds and name ...
            ldy         #RFR_FD
:
            lda         TA_FD,X
            sta         (K_PTR2),Y
            iny
            inx
            cpx         #FD_MAX
            bne         :-
            ldy         #RE_ZP                              ; ... and its zero page where the program's goes
            lda         (K_PTR),Y
            tax
            beq         @zpdone
            ldy         #RFR_ZP
            lda         #PROG_ZP
            sta         K_TMP
:
            phx
            ldx         K_TMP
            lda         0,X
            stz         0,X                                 ; (The program's: cleared)
            inc         K_TMP
            plx
            sta         (K_PTR2),Y
            iny
            dex
            bne         :-
@zpdone:
            lda         R_BANK                              ; ---- The task the program's
            sta         TK_RBANK
            lda         K_PTR2
            sta         TK_RFRAME
            lda         K_PTR2 + 1
            sta         TK_RFRAME + 1
            stz         TA_NOTIFY
            stz         TA_NOTIFY + 1
            stz         TK_INNOTE
            stz         TK_PREEMPT
            lda         L_FLAGS                             ; Its break: on from the caller's (RUN_LEND), else
            and         #RUN_LEND                           ;   none to have
            bne         :+
            lda         #<RB_MODS
            sta         TA_BRKMIN
            lda         #>RB_MODS
            sta         TA_BRKMIN + 1
            bra         @name
:
            lda         TA_BRK
            sta         TA_BRKMIN
            lda         TA_BRK + 1
            sta         TA_BRKMIN + 1
@name:
            ldy         #RE_START                           ; Its main (its image's header's)
            lda         (K_PTR),Y
            sta         K_PTR2
            iny
            lda         (K_PTR),Y
            sta         K_PTR2 + 1
            ldy         #HX_MAIN
            lda         (K_PTR2),Y
            sta         R_ENTRY
            iny
            lda         (K_PTR2),Y
            sta         R_ENTRY + 1
            jsr         run_fds                             ; Its fds
            lda         L_ARGS                              ; Its arguments (none: the frame's 0)
            ora         L_ARGS + 1
            beq         :+
            lda         L_ARGS
            sta         r0
            lda         L_ARGS + 1
            sta         r0 + 1
            rts
:
            clc
            lda         TK_RFRAME
            adc         #RFR_NIL
            sta         r0
            lda         TK_RFRAME + 1
            adc         #0
            sta         r0 + 1
            rts

; The program's fds: the caller's 0, 1 and 2, or the map's (R_MAP: a count, then the caller's fd for each of the
; program's from 0), from the frame's copy of the caller's; each channel counted again (the kernel task's)
run_fds:
            ldx         #FD_MAX - 1
            lda         #$FF
:
            sta         TA_FD,X
            dex
            bpl         :-
            lda         L_FLAGS
            and         #SPAWN_FDMAP
            bne         @map
            ldx         #2                                  ; (None: 0, 1 and 2)
            ldy         #RFR_FD + 2
:
            lda         (TK_RFRAME),Y
            sta         TA_FD,X
            dey
            dex
            bpl         :-
            bra         @refs

@map:
            lda         (R_MAP)                             ; (Its count: SPAWN_FDS at most)
            cmp         #SPAWN_FDS + 1
            bcc         :+
            lda         #SPAWN_FDS
:
            sta         K_TMP
            ldx         #0
@fd:
            cpx         K_TMP
            beq         @refs
            phx
            txa
            tay
            iny
            lda         (R_MAP),Y                           ; (The caller's fd for it)
            plx
            cmp         #FD_MAX
            bcs         @next
            clc
            adc         #RFR_FD
            tay
            lda         (TK_RFRAME),Y
            sta         TA_FD,X
@next:
            inx
            bra         @fd

@refs:
            KCALL_FAR   K_RUN_FDREF
            rts

; ****************************************************************************
; RUN's end (K_RUN_EXIT's: main returned, or EXITS, a note's default too, in the program running in place, its
; message in TA_SCRATCH already): its fds closed and the caller's put back with the rest of what its frame kept, the
; note that ended it (but a kill) posted again.  IN: .A = its code.  OUT: .A = the code; .X = the stack RUN returns
; with
K_RUN_END:
            pha
            lda         TK_RBANK                            ; (Its bank: its frame's)
            sta         RAM_BANK
            FARCALL     K_CLOSE_ALL                         ; Its fds; the caller's again
            ldx         #0
            ldy         #RFR_FD
:
            lda         (TK_RFRAME),Y
            sta         TA_FD,X
            iny
            inx
            cpx         #FD_MAX
            bne         :-
            ldy         #RFR_ROMB                           ; Its bank at $A000, note handler, break ...
            lda         (TK_RFRAME),Y
            sta         ROM_BANK
            iny
            lda         (TK_RFRAME),Y
            sta         TA_NOTIFY
            iny
            lda         (TK_RFRAME),Y
            sta         TA_NOTIFY + 1
            iny
            lda         (TK_RFRAME),Y
            sta         TA_BRK
            iny
            lda         (TK_RFRAME),Y
            sta         TA_BRK + 1
            iny
            lda         (TK_RFRAME),Y
            sta         TA_BRKMIN
            iny
            lda         (TK_RFRAME),Y
            sta         TA_BRKMIN + 1
            iny
            lda         (TK_RFRAME),Y
            sta         TK_PREEMPT
            iny
            lda         (TK_RFRAME),Y
            sta         TK_INNOTE
            ldy         #RFR_ENT                            ; The module: not running, used now
            lda         (TK_RFRAME),Y
            sta         R_ENT
            lda         TK_RBANK
            sta         R_BANK
            jsr         re_at
            ldy         #RE_FLAGS
            lda         (K_PTR),Y
            and         #<~RF_BUSY
            sta         (K_PTR),Y
            jsr         re_stamp
            ldy         #RE_ZP                              ; ... the zero page the program had
            lda         (K_PTR),Y
            tax
            beq         @zpdone
            lda         #PROG_ZP
            sta         K_TMP
            ldy         #RFR_ZP
:
            lda         (TK_RFRAME),Y
            phx
            ldx         K_TMP
            sta         0,X
            inc         K_TMP
            plx
            iny
            dex
            bne         :-
@zpdone:
            lda         TK_RNOTE                            ; The note that ended it, posted again (but a kill)
            beq         @frame
            stz         TK_RNOTE
            cmp         #NOTE_KILL
            beq         @frame
            jsr         rn_post
@frame:
            ldy         #RFR_S                              ; ---- The run it was in (if any)
            lda         (TK_RFRAME),Y
            sta         K_TMP
            ldy         #RFR_RAMB
            lda         (TK_RFRAME),Y
            sta         K_TMP2
            ldy         #RFR_PBANK
            lda         (TK_RFRAME),Y
            pha
            ldy         #RFR_PFRAME + 1
            lda         (TK_RFRAME),Y
            tax
            dey
            lda         (TK_RFRAME),Y
            sta         TK_RFRAME
            stx         TK_RFRAME + 1
            pla
            sta         TK_RBANK
            lda         K_TMP2
            sta         RAM_BANK
            cli
            pla
            ldx         K_TMP
            rts

; Note .A (1-31) pending for this task (notes.s: K_NOTE_TAKE takes it)
rn_post:
            tay
            and         #7
            tax
            lda         rn_bits,X
            pha
            tya
            lsr
            lsr
            lsr
            tax
            pla
            php
            sei
            ora         TK_NOTES,X
            sta         TK_NOTES,X
            lda         #1
            sta         TK_NOTED
            plp
            rts

; ****************************************************************************
; In the kernel task (KCALL, SPAWN_RUN's): the caller's (.Y's) exit record, for its own WAIT: code .A, its message
; in its TA_SCRATCH, its parent itself
K_RUN_RECORD:
            sta         K_EXIT_CODE,Y
            lda         #EX_HELD
            sta         K_EXIT_STATE,Y
            tya
            sta         K_EXIT_PARENT,Y
            sty         K0_TMP
            FARCALL     K_EXIT_MSG_AT                       ; (K_PTR2: its message's place)
            lda         K_PTR2
            sta         K_PTR
            lda         K_PTR2 + 1
            sta         K_PTR + 1
            lda         #<TA_SCRATCH
            sta         K_PTR2
            lda         #>TA_SCRATCH
            sta         K_PTR2 + 1
            lda         #EXIT_MSG_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_TMP
            sec
            FARCALL     K_KCOPY
            clc
            rts

; In the kernel task (KCALL, RUN's): each channel the caller's (.Y's) fds name counted once more
K_RUN_FDREF:
            ldx         #FD_MAX - 1
@fd:
            php
            sei
            sty         T_REGISTER
            lda         TA_FD,X
            stz         T_REGISTER
            plp
            cmp         #CH_MAX
            bcs         @next
            phx
            tax
            inc         K_CH_REFS,X
            plx
@next:
            dex
            bpl         @fd
            clc
            rts

; ****************************************************************************
; A bank program in a task of its own (K_BANK_START's, in the new task): loaded from its file (LOAD_FD, at its o65
; module: K_LOAD read its header) into a bank of the task's, which the window has from here on; its zero page
; cleared (the task's own).  OUT: C = 0, R_ENTRY its main; or C = 1, .A = the error
K_BANK_TASKLOAD:
            lda         #LOAD_FD
            sta         L_FD
            lda         #HX_SIZE
            sta         L_TASK
            stz         R_ID                                ; (No identity: RUN never finds it)
            stz         R_ID + 1
            stz         R_ID + 2
            stz         R_ID + 3
            lda         #RF_PROG
            sta         R_ENTRY + 1
            lda         #PROG_ZP
            sta         R_ENTRY
            jsr         rl_pre                              ; (Its header, text and data in one read, if it can)
            bcs         :+
            jsr         rl_body
            bra         :++
:
            jsr         rl_load
:
            php
            pha
            lda         L_FD
            FARCALL     K_CLOSE
            pla
            plp
            bcs         @done
            jsr         re_at
            ldy         #RE_FLAGS                           ; (Running: never given up)
            lda         (K_PTR),Y
            ora         #RF_BUSY
            sta         (K_PTR),Y
            ldx         #PROG_ZP
:
            stz         0,X
            inx
            cpx         #PROG_ZP_END
            bne         :-
            ldy         #RE_START                           ; Its main
            lda         (K_PTR),Y
            sta         K_PTR2
            iny
            lda         (K_PTR),Y
            sta         K_PTR2 + 1
            ldy         #HX_MAIN
            lda         (K_PTR2),Y
            sta         R_ENTRY
            iny
            lda         (K_PTR2),Y
            sta         R_ENTRY + 1
            clc
@done:
            rts

; A bank program's own task's load made quicker (K_BANK_TASKLOAD's): its o65 header, text and data read at once, its
; text landing at LINKED in the cache's first bank (empty: the task is new) and its header just below, in the task's
; RAM (nothing of the program's there yet; cleared after), when its file's HYX2 header (in TA_PATH: K_LOAD's) gives
; their length (HX_LENGTH, the build's: the file's bytes to its data's end) and the o65 header is the build's (its
; options only the 0 that ends them).  OUT: C = 0, RS_HDR and RS_TEXT (its high byte $FF: the text's read already),
; the first bank in the window; or C = 1 (read it as RUN reads one: rl_load)
RL_PRE      = LINKED - O65_HSIZE - 1
RL_LEN      = HX_SIZE + O65_HSIZE + 1   ; (The headers', the HYX2 and the o65, with its options' end)
rl_pre:
            jsr         rc_first
            bcs         @rts
            sec                                             ; Its text's and data's length, under a bank's room
            lda         TA_PATH + HX_LENGTH
            sbc         #<RL_LEN
            sta         r1
            lda         TA_PATH + HX_LENGTH + 1
            sbc         #>RL_LEN
            sta         r1 + 1
            bcc         @rts
            cmp         #>(RB_TOP - LINKED)
            bcs         @rts
            clc                                             ; ... and the o65 header's
            lda         r1
            adc         #O65_HSIZE + 1
            sta         r1
            bcc         :+
            inc         r1 + 1
:
            lda         #<RL_PRE
            sta         r0
            lda         #>RL_PRE
            sta         r0 + 1
            jsr         rl_readx                            ; (The file's at its HYX2 header's end: K_LOAD's)
            bcs         @rts
            ldx         #O65_HSIZE - 1                      ; Its header, then the task's RAM as it was
:
            lda         RL_PRE,X
            sta         RS_HDR,X
            stz         RL_PRE,X
            dex
            bpl         :-
            lda         RL_PRE + O65_HSIZE                  ; (No options)
            stz         RL_PRE + O65_HSIZE
            bne         @no
            clc                                             ; (Its text and data the bytes read: HX_LENGTH)
            lda         RS_HDR + O65_TLEN
            adc         RS_HDR + O65_DLEN
            tax
            lda         RS_HDR + O65_TLEN + 1
            adc         RS_HDR + O65_DLEN + 1
            tay
            clc
            txa
            adc         #<RL_LEN
            tax
            tya
            adc         #>RL_LEN
            cmp         TA_PATH + HX_LENGTH + 1
            bne         @no
            cpx         TA_PATH + HX_LENGTH
            bne         @no
            lda         #RL_LEN
            sta         RS_TEXT
            stz         RS_TEXT + 1
            jsr         rl_form
            bcs         @rts
            lda         #$FF
            sta         RS_TEXT + 1
            clc
@rts:
            rts

@no:
            sec
            rts

; ****************************************************************************
; The cache

; R_ID = the identity of the file at L_PATH, from its STAT record (in the first cache bank's scratch): a hash of its
; qid, version, length, time and device; never 0.  OUT: C = 1, .A = the error (STAT's, or no bank for the cache)
rc_stat:
            jsr         rc_first
            bcs         @done
            lda         L_PATH
            sta         r0
            lda         L_PATH + 1
            sta         r0 + 1
            lda         #<RS_BUF
            sta         r1
            lda         #>RS_BUF
            sta         r1 + 1
            FARCALL     K_STAT
            bcs         @done
            ldx         #3                                  ; Its qid's path first ...
:
            lda         RS_BUF + SR_QPATH,X
            sta         R_ID,X
            dex
            bpl         :-
            ldy         #0                                  ; ... then the rest, each rotated in
@byte:
            ldx         rc_idbytes,Y
            bmi         @end
            lda         RS_BUF,X
            pha
            tya
            and         #3
            tax
            lda         R_ID,X
            asl
            adc         #0
            sta         R_ID,X
            pla
            eor         R_ID,X
            sta         R_ID,X
            iny
            bra         @byte

@end:
            lda         R_ID                                ; (Never 0: no identity's)
            ora         R_ID + 1
            ora         R_ID + 2
            ora         R_ID + 3
            bne         :+
            inc         R_ID
:
            clc
@done:
            rts

; The cache's first bank (one made if there's none), in the window.  OUT: C = 0, .A = it; or C = 1, .A = the error
rc_first:
            lda         TK_RCACHE
            cmp         #$FF
            bne         @map
            jsr         rc_newbank
            bcs         @done
            sta         TK_RCACHE
@map:
            sta         RAM_BANK
            clc
@done:
            rts

; A new cache bank: allocated (BANKS_ALLOC), in the window, its directory empty.  OUT: C = 0, .A = it; or C = 1,
; .A = the error
rc_newbank:
            lda         #1
            FARCALL     K_BANKS_ALLOC
            bcs         @done
            sta         RAM_BANK
            ldx         #0
            lda         #0
:
            sta         RB_DIR,X
            inx
            bne         :-
            lda         #$FF
            sta         RB_NEXT
            lda         RAM_BANK
            clc
@done:
            rts

; The module in the cache of kind .A (RF_PROG, or 0: a child) whose file is R_ID's and whose zero page is at .X.
; OUT: C = 0, R_BANK and R_ENT it (its bank in the window); or C = 1
rc_find:
            sta         K_TMP
            stx         K_TMP2
            lda         TK_RCACHE
@bank:
            cmp         #$FF
            beq         @none
            sta         RAM_BANK
            sta         R_BANK
            ldx         #RB_ENTS
@ent:
            jsr         re_atx
            ldy         #RE_GEN
            lda         (K_PTR),Y
            beq         @next
            ldy         #RE_FLAGS
            lda         (K_PTR),Y
            and         #RF_PROG
            cmp         K_TMP
            bne         @next
            ldy         #RE_ZPAT
            lda         (K_PTR),Y
            cmp         K_TMP2
            bne         @next
            ldy         #RE_ID + 3
:
            lda         (K_PTR),Y
            cmp         R_ID - RE_ID,Y
            bne         @next
            dey
            cpy         #RE_ID
            bcs         :-
            stx         R_ENT
            clc
            rts

@next:
            dex
            bne         @ent
            lda         RB_NEXT
            bra         @bank

@none:
            sec
            rts

; K_PTR = directory entry R_ENT of bank R_BANK (which the window has after).  Keeps .A
re_at:
            pha
            lda         R_BANK
            sta         RAM_BANK
            ldx         R_ENT
            pla
; K_PTR = directory entry .X of the bank in the window.  Keeps .A, .X, .Y
re_atx:
            pha
            txa
            asl
            asl
            asl
            asl
            sta         K_PTR
            lda         #>RB_DIR
            sta         K_PTR + 1
            pla
            rts

; Entry K_PTR used now (TK_RCLOCK on one).  Keeps .X
re_stamp:
            inc         TK_RCLOCK
            bne         :+
            inc         TK_RCLOCK + 1
:
            ldy         #RE_STAMP
            lda         TK_RCLOCK
            sta         (K_PTR),Y
            iny
            lda         TK_RCLOCK + 1
            sta         (K_PTR),Y
            rts

; A place for L_AT bytes (a module), and a free entry for it: in the first cache bank with room, else in a new bank
; (TK_RMAX, or RC_BANKS, at most), else after giving up the modules used least lately.  OUT: C = 0, L_AT the place,
; R_BANK and R_ENT (its bank in the window; the entry its START and END, its GEN still 0); or C = 1, .A = E_TOOBIG
; (more than a bank's room), E_NOMEM
rc_place:
            lda         L_AT                                ; (A bank's room at most)
            cmp         #<(RB_TOP - RB_MODS + 1)
            lda         L_AT + 1
            sbc         #>(RB_TOP - RB_MODS + 1)
            bcc         @try
            lda         #E_TOOBIG
            rts

@try:
            stz         K_TMP                               ; (The banks counted)
            lda         TK_RCACHE
@bank:
            cmp         #$FF
            beq         @new
            sta         RAM_BANK
            sta         R_BANK
            inc         K_TMP
            jsr         rc_gap
            bcc         @got
            lda         RB_NEXT
            bra         @bank

@new:
            lda         TK_RMAX                             ; Another bank, if the cache may have one
            bne         :+
            lda         #RC_BANKS
:
            cmp         K_TMP
            beq         @evict
            bcc         @evict
            jsr         rc_newbank
            bcs         @evict
            pha                                             ; (After the last: R_BANK's)
            lda         R_BANK
            sta         RAM_BANK
            pla
            sta         RB_NEXT
            sta         RAM_BANK
            sta         R_BANK
            jsr         rc_gap
            bcc         @got
@evict:
            jsr         rc_lru                              ; (None to give up: no room)
            bcc         @try
            lda         #E_NOMEM
            rts

@got:
            stx         R_ENT
            jsr         re_atx
            ldy         #RE_START
            lda         L_LEFT
            sta         (K_PTR),Y
            iny
            lda         L_LEFT + 1
            sta         (K_PTR),Y
            iny                                             ; (RE_END)
            clc
            lda         L_LEFT
            adc         L_AT
            sta         (K_PTR),Y
            iny
            lda         L_LEFT + 1
            adc         L_AT + 1
            sta         (K_PTR),Y
            ldy         #RE_GEN
            lda         #0
            sta         (K_PTR),Y
            lda         L_LEFT
            sta         L_AT
            lda         L_LEFT + 1
            sta         L_AT + 1
            clc
            rts

; The lowest gap of L_AT bytes in the bank in the window, and a free entry.  OUT: C = 0, L_LEFT the gap, .X the
; entry; or C = 1
rc_gap:
            ldx         #RB_ENTS                            ; A free entry
:
            jsr         re_atx
            ldy         #RE_GEN
            lda         (K_PTR),Y
            beq         @entry
            dex
            bne         :-
            sec
            rts

@entry:
            stx         K_TMP2
            lda         #<RB_MODS                           ; From the bank's start, past each module it meets
            sta         L_LEFT
            lda         #>RB_MODS
            sta         L_LEFT + 1
@scan:
            clc                                             ; (K_CNT: the gap's end)
            lda         L_LEFT
            adc         L_AT
            sta         K_CNT
            lda         L_LEFT + 1
            adc         L_AT + 1
            sta         K_CNT + 1
            ldx         #RB_ENTS
@ent:
            jsr         re_atx
            ldy         #RE_GEN
            lda         (K_PTR),Y
            beq         @next
            ldy         #RE_END                             ; Its end above the gap's start ...
            lda         L_LEFT
            cmp         (K_PTR),Y
            iny
            lda         L_LEFT + 1
            sbc         (K_PTR),Y
            bcs         @next
            ldy         #RE_START                           ; ... and its start below the gap's end: past it
            lda         (K_PTR),Y
            cmp         K_CNT
            iny
            lda         (K_PTR),Y
            sbc         K_CNT + 1
            bcs         @next
            ldy         #RE_END
            lda         (K_PTR),Y
            sta         L_LEFT
            iny
            lda         (K_PTR),Y
            sta         L_LEFT + 1
            bra         @scan

@next:
            dex
            bne         @ent
            lda         #<RB_TOP                            ; Below the modules' top?
            cmp         K_CNT
            lda         #>RB_TOP
            sbc         K_CNT + 1
            bcc         :+
            ldx         K_TMP2
            clc
            rts
:
            sec
            rts

; The module used least lately (and not running) given up: its entry free.  OUT: C = 1 if there's none
rc_lru:
            lda         #$FF                                ; (K_TMP2: its bank so far; K_CNT its stamp; .Y the entry)
            sta         K_TMP2
            lda         TK_RCACHE
@bank:
            cmp         #$FF
            beq         @done
            sta         RAM_BANK
            ldx         #RB_ENTS
@ent:
            jsr         re_atx
            ldy         #RE_GEN
            lda         (K_PTR),Y
            beq         @next
            ldy         #RE_FLAGS
            lda         (K_PTR),Y
            and         #RF_BUSY
            bne         @next
            lda         K_TMP2                              ; (The first; or older than the best)
            cmp         #$FF
            beq         @take
            ldy         #RE_STAMP
            lda         (K_PTR),Y
            cmp         K_CNT
            iny
            lda         (K_PTR),Y
            sbc         K_CNT + 1
            bcs         @next
@take:
            lda         RAM_BANK
            sta         K_TMP2
            stx         K_TMP
            ldy         #RE_STAMP
            lda         (K_PTR),Y
            sta         K_CNT
            iny
            lda         (K_PTR),Y
            sta         K_CNT + 1
@next:
            dex
            bne         @ent
            lda         RB_NEXT
            bra         @bank

@done:
            lda         K_TMP2
            cmp         #$FF
            beq         @none
            sta         RAM_BANK
            ldx         K_TMP
            jsr         re_atx
            ldy         #RE_GEN
            lda         #0
            sta         (K_PTR),Y
            clc
            rts

@none:
            sec
            rts

; ****************************************************************************
; The loader

; The program at L_PATH loaded into the cache (RUN's: a bank program that may run in place), R_ID its identity.  OUT:
; C = 0, R_BANK and R_ENT it (its bank in the window); or C = 1, .A = the error (E_NOEXEC: not one)
rl_prog:
            lda         L_PATH
            sta         r0
            lda         L_PATH + 1
            sta         r0 + 1
            lda         #O_READ
            FARCALL     K_OPEN
            bcs         @done
            sta         L_FD
            lda         #RF_PROG                            ; (Its kind and zero page)
            sta         R_ENTRY + 1
            lda         #PROG_ZP
            sta         R_ENTRY
            lda         #HX_SIZE                            ; Its HYX2 header, then its o65 module (in the first
            sta         L_TASK                              ;   bank's scratch)
            jsr         rc_first
            bcs         @close
            jsr         rl_head
            bcs         @close
            ldx         #3
:
            lda         RS_BUF + HX_MAGIC,X
            cmp         rl_hyx2,X
            bne         @noexec
            dex
            bpl         :-
            lda         RS_BUF + HX_TYPE
            cmp         #HT_PROGRAM
            bne         @noexec
            lda         RS_BUF + HX_FLAGS                   ; (A bank program, not one for a task alone)
            and         #HF_BANK | HF_TASK
            cmp         #HF_BANK
            bne         @noexec
            jsr         rl_body
            bra         @close

@noexec:
            lda         #E_NOEXEC
            sec
@close:
            php
            pha
            lda         L_FD
            FARCALL     K_CLOSE
            pla
            plp
@done:
            rts

; A module from L_FD (its o65 module at L_TASK in the file) into the cache.  IN: R_ENTRY + 1 = RF_PROG (a program: no
; imports; its run frame after its BSS) or 0 (a child); R_ENTRY = where its zero page goes; R_ID its file's identity.
; OUT: C = 0, R_BANK and R_ENT it (its bank in the window, the entry made but its stamp); or C = 1, .A = the error
; (E_NOEXEC: not a module this loader takes)
rl_load:
            jsr         rc_first                            ; Its header, in the first bank's scratch
            jcs         rl_rts
            jsr         rl_head
            jcs         rl_rts
; The module whose header rl_head read (into the scratch of the bank in the window), loaded: its place found, its
; text and data read there, the rest of the file (its imports, its relocations) read through the room past them (its
; BSS and frame, cleared after).  OUT: as rl_load's
rl_body:
            clc                                             ; Its room: its text, data and BSS ...
            lda         RS_HDR + O65_TLEN
            adc         RS_HDR + O65_DLEN
            sta         L_AT
            lda         RS_HDR + O65_TLEN + 1
            adc         RS_HDR + O65_DLEN + 1
            sta         L_AT + 1
            clc
            lda         L_AT
            adc         RS_HDR + O65_BLEN
            sta         L_AT
            lda         L_AT + 1
            adc         RS_HDR + O65_BLEN + 1
            sta         L_AT + 1
            jcs         rl_big
            lda         R_ENTRY + 1                         ; ... and a program's frame (its zero page's room), or
            beq         @child                              ;   a child's tables'
            clc
            lda         #RFR_SIZE
            adc         RS_HDR + O65_ZLEN
            adc         L_AT
            sta         L_AT
            bcc         :+
            inc         L_AT + 1
:
            clc                                             ; (And a copy of its data)
            lda         L_AT
            adc         RS_HDR + O65_DLEN
            sta         L_AT
            lda         L_AT + 1
            adc         RS_HDR + O65_DLEN + 1
            sta         L_AT + 1
            jcs         rl_big
            bra         @room
@child:
            sec                                             ; (CHILD_ROOM, or what the bank has past it)
            lda         #<(RB_TOP - RB_MODS)
            sbc         L_AT
            tax
            lda         #>(RB_TOP - RB_MODS)
            sbc         L_AT + 1
            jcc         rl_big
            cmp         #>CHILD_ROOM
            bcc         :+
            lda         #>CHILD_ROOM
            ldx         #<CHILD_ROOM
:
            pha
            txa
            clc
            adc         L_AT
            sta         L_AT
            pla
            adc         L_AT + 1
            sta         L_AT + 1
@room:
            ldx         #RS_HSIZE - 1                       ; ---- Its place: its header (and its text's place)
:                                                           ;   on the stack meanwhile, then in the scratch of
            lda         RS_HDR,X                            ;   the place's bank
            pha
            dex
            bpl         :-
            jsr         rc_place
            sta         K_TMP2
            php
            pla
            sta         K_TMP
            ldx         #0
:
            pla
            sta         RS_HDR,X
            inx
            cpx         #RS_HSIZE
            bne         :-
            lda         K_TMP
            pha
            lda         K_TMP2
            plp
            jcs         rl_rts
            lda         R_ENTRY
            sta         RS_ZPAT
            sec
            sbc         RS_HDR + O65_ZBASE
            sta         RS_ZPD
            jsr         re_at                               ; (Its room's end)
            ldy         #RE_END
            lda         (K_PTR),Y
            sta         RS_LIM
            iny
            lda         (K_PTR),Y
            sta         RS_LIM + 1
            lda         R_ENTRY + 1
            sta         RS_KIND
            sec
            lda         L_AT
            sbc         #<LINKED
            sta         RS_DELTA
            lda         L_AT + 1
            sbc         #>LINKED
            sta         RS_DELTA + 1
            lda         RS_TEXT + 1                         ; (Its text and data read already, at LINKED in the
            cmp         #$FF                                ;   first bank: rl_pre's; or read here after all, its
            bne         @readit                             ;   place another)
            stz         RS_TEXT + 1
            lda         L_AT
            bne         @readit
            lda         L_AT + 1
            cmp         #>LINKED
            bne         @readit
            lda         RAM_BANK
            cmp         TK_RCACHE
            beq         @inplace
@readit:
            lda         RS_TEXT                             ; Its text and data
            sta         r0
            lda         RS_TEXT + 1
            sta         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #0
            lda         L_FD
            FARCALL     K_SEEK
            jcs         rl_rts
            lda         L_AT
            sta         r0
            lda         L_AT + 1
            sta         r0 + 1
            clc
            lda         RS_HDR + O65_TLEN
            adc         RS_HDR + O65_DLEN
            sta         r1
            lda         RS_HDR + O65_TLEN + 1
            adc         RS_HDR + O65_DLEN + 1
            sta         r1 + 1
            jsr         rl_readx
            jcs         rl_rts
@inplace:
            clc                                             ; (RS_END: the image's end, its BSS's start)
            lda         L_AT
            adc         RS_HDR + O65_TLEN
            tax
            lda         L_AT + 1
            adc         RS_HDR + O65_TLEN + 1
            sta         RS_END + 1
            clc
            txa
            adc         RS_HDR + O65_DLEN
            sta         RS_END
            lda         RS_END + 1
            adc         RS_HDR + O65_DLEN + 1
            sta         RS_END + 1
            lda         RS_DELTA                            ; (A program where it was linked: nothing to move, so
            ora         RS_DELTA + 1                        ;   its relocations aren't read)
            ora         RS_ZPD
            bne         :+
            lda         RS_KIND
            beq         :+
            jmp         @bss
:
            lda         RS_KIND                             ; The rest: a child's through the scratch's buffer
            bne         :+                                  ;   (its room past the BSS is its tables')
            lda         #<RS_BUF
            sta         RS_BP
            lda         #>RS_BUF
            sta         RS_BP + 1
            lda         #RS_BUFLEN
            sta         RS_BSZ
            stz         RS_BSZ + 1
            bra         @buffer
:
            lda         RS_END                              ; ... a program's through the room past its text and
            sta         RS_BP                               ;   data (RS_BP: its BSS's start)
            lda         RS_END + 1
            sta         RS_BP + 1
            sec
            lda         RS_LIM
            sbc         RS_BP
            sta         RS_BSZ
            lda         RS_LIM + 1
            sbc         RS_BP + 1
            sta         RS_BSZ + 1
@buffer:
            stz         K_CNT                               ; (rl_byte's buffer empty)
            stz         K_CNT + 1
            jsr         rl_imports                          ; Its imports (a child's)
            jcs         rl_rts
            sec                                             ; Its text's relocations, then its data's
            lda         L_AT
            sbc         #1
            sta         L_LEFT
            lda         L_AT + 1
            sbc         #0
            sta         L_LEFT + 1
            jsr         rl_reloc
            jcs         rl_rts
            clc
            lda         L_AT
            adc         RS_HDR + O65_TLEN
            sta         L_LEFT
            lda         L_AT + 1
            adc         RS_HDR + O65_TLEN + 1
            sta         L_LEFT + 1
            lda         L_LEFT
            bne         :+
            dec         L_LEFT + 1
:
            dec         L_LEFT
            jsr         rl_reloc
            jcs         rl_rts
            lda         RS_KIND                             ; A child's exports' table
            bne         @bss
            jsr         rl_exports
            jcs         rl_rts
@bss:
            lda         RS_END                              ; Its BSS cleared (RS_END: past it)
            sta         K_PTR2
            lda         RS_END + 1
            sta         K_PTR2 + 1
            ldx         RS_HDR + O65_BLEN + 1
            ldy         #0
            lda         #0
            cpx         #0
            beq         @part
@page:
            sta         (K_PTR2),Y
            iny
            bne         @page
            inc         K_PTR2 + 1
            dex
            bne         @page
@part:
            cpy         RS_HDR + O65_BLEN
            beq         :+
            sta         (K_PTR2),Y
            iny
            bra         @part
:
            tya
            clc
            adc         K_PTR2
            sta         RS_END
            lda         K_PTR2 + 1
            adc         #0
            sta         RS_END + 1
            lda         RS_KIND                             ; A program's frame: where its data is, its BSS, and its
            beq         @entry                              ;   data's copy (past the frame), made now
            jsr         rl_frame
@entry:
            jsr         re_at                               ; ---- Its entry
            ldy         #RE_TABLE                           ; (Its frame: past its BSS)
            lda         RS_END
            sta         (K_PTR),Y
            iny
            lda         RS_END + 1
            sta         (K_PTR),Y
            inc         TK_RGEN                             ; (Its load's number: never 0)
            bne         :+
            inc         TK_RGEN
:
            ldy         #RE_GEN
            lda         TK_RGEN
            sta         (K_PTR),Y
            iny
            lda         RS_KIND
            sta         (K_PTR),Y
            bne         :+
            ldy         #RE_END                             ; (A child's end: its table's)
            lda         RS_TEND
            sta         (K_PTR),Y
            iny
            lda         RS_TEND + 1
            sta         (K_PTR),Y
:
            ldy         #RE_ID
            ldx         #0
:
            lda         R_ID,X
            sta         (K_PTR),Y
            iny
            inx
            cpx         #4
            bne         :-
            lda         RS_HDR + O65_ZLEN                   ; (RE_ZP, RE_ZPAT)
            sta         (K_PTR),Y
            iny
            lda         RS_ZPAT
            sta         (K_PTR),Y
            clc
rl_rts:
            rts

rl_big:
            lda         #E_TOOBIG
            sec
            rts

rl_noexec:
            lda         #E_NOEXEC
            sec                                             ; (Its entry left free: GEN 0)
            rts

; A program's frame (at RS_END, past its BSS) told where its data is (L_AT + its text), how long it and its BSS are,
; and where the copy of its data is (past the frame and its zero page's room); the copy made
rl_frame:
            lda         RS_END
            sta         K_PTR2
            lda         RS_END + 1
            sta         K_PTR2 + 1
            ldy         #RFM_DATA
            clc
            lda         L_AT
            adc         RS_HDR + O65_TLEN
            sta         (K_PTR2),Y
            iny
            lda         L_AT + 1
            adc         RS_HDR + O65_TLEN + 1
            sta         (K_PTR2),Y
            iny                                             ; (RFM_DLEN, RFM_BLEN)
            lda         RS_HDR + O65_DLEN
            sta         (K_PTR2),Y
            iny
            lda         RS_HDR + O65_DLEN + 1
            sta         (K_PTR2),Y
            iny
            lda         RS_HDR + O65_BLEN
            sta         (K_PTR2),Y
            iny
            lda         RS_HDR + O65_BLEN + 1
            sta         (K_PTR2),Y
            iny                                             ; (RFM_COPY)
            clc
            lda         K_PTR2
            adc         #RFR_SIZE
            php
            clc
            adc         RS_HDR + O65_ZLEN
            sta         (K_PTR2),Y
            iny
            lda         K_PTR2 + 1
            adc         #0
            plp
            adc         #0
            sta         (K_PTR2),Y
            ldx         #0                                  ; The copy: data to it
; Its data copied (.X = 0: to its copy; 1: from it, a new run's), its BSS cleared too (.X = 1).  IN: K_PTR2 its frame
rf_data:
            ldy         #RFM_COPY                           ; (r0 the copy, r1 the data, r2 the length)
            lda         (K_PTR2),Y
            sta         r0
            iny
            lda         (K_PTR2),Y
            sta         r0 + 1
            ldy         #RFM_DATA
            lda         (K_PTR2),Y
            sta         r1
            iny
            lda         (K_PTR2),Y
            sta         r1 + 1
            iny
            lda         (K_PTR2),Y
            sta         r2
            iny
            lda         (K_PTR2),Y
            sta         r2 + 1
            txa
            bne         @from
            lda         r0                                  ; (To the copy: r0 and r1 the other way)
            ldy         r1
            sta         r1
            sty         r0
            lda         r0 + 1
            ldy         r1 + 1
            sta         r1 + 1
            sty         r0 + 1
@from:
            phx
            jsr         rf_move                             ; (r2 bytes from r0 to r1)
            plx
            txa
            beq         @done
            ldy         #RFM_BLEN                           ; Its BSS (just before the frame) cleared
            sec
            lda         K_PTR2
            sbc         (K_PTR2),Y
            sta         r1
            iny
            lda         K_PTR2 + 1
            sbc         (K_PTR2),Y
            sta         r1 + 1
            dey
            lda         (K_PTR2),Y
            sta         r2
            iny
            lda         (K_PTR2),Y
            sta         r2 + 1
            ldx         #0
            ldy         #0
@page:
            lda         r2 + 1
            beq         @part
@bytes:
            txa
            sta         (r1),Y
            iny
            bne         @bytes
            inc         r1 + 1
            dec         r2 + 1
            bra         @page
@part:
            cpy         r2
            beq         @done
            txa
            sta         (r1),Y
            iny
            bra         @part
@done:
            rts

; r2 bytes from r0 to r1 (not overlapping)
rf_move:
            ldy         #0
@page:
            lda         r2 + 1
            beq         @part
@bytes:
            lda         (r0),Y
            sta         (r1),Y
            iny
            bne         @bytes
            inc         r0 + 1
            inc         r1 + 1
            dec         r2 + 1
            bra         @page
@part:
            cpy         r2
            beq         @done
            lda         (r0),Y
            sta         (r1),Y
            iny
            bra         @part
@done:
            rts

; The file's first RS_BUFLEN bytes read into RS_BUF (the scratch of the bank in the window): its o65 module's header
; (at L_TASK in them) into RS_HDR and held to the form this loader takes (o65, not the 65816's, paged, 32-bit nor
; an object file; its text at LINKED, its data after it, its BSS after that; its zero page at 0, a program's the
; program's room at most), its options passed over: RS_TEXT its text's place in the file.  OUT: C = 1, .A = the
; error (E_NOEXEC)
rl_head:
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #0
            lda         L_FD
            FARCALL     K_SEEK
            jcs         @rts
            lda         #<RS_BUF
            sta         r0
            lda         #>RS_BUF
            sta         r0 + 1
            lda         #RS_BUFLEN
            sta         r1
            stz         r1 + 1
            lda         L_FD
            FARCALL     K_READ
            jcs         @rts
            sta         K_TMP                               ; (The bytes there)
            sec
            sbc         #O65_HSIZE
            jcc         @noexec
            cmp         L_TASK
            jcc         @noexec
            ldx         L_TASK                              ; Its header
            ldy         #0
:
            lda         RS_BUF,X
            sta         RS_HDR,Y
            inx
            iny
            cpy         #O65_HSIZE
            bne         :-
@option:                                                    ; Its options (each its length, itself counted; a 0
            cpx         K_TMP                               ;   after the last), within the bytes read
            jcs         @noexec
            lda         RS_BUF,X
            beq         @text
            cmp         #2
            jcc         @noexec
            sta         K_TMP2
            txa
            clc
            adc         K_TMP2
            tax
            jcs         @noexec
            bra         @option
@text:
            inx
            stx         RS_TEXT
            stz         RS_TEXT + 1
            bra         rl_form

@noexec:
            lda         #E_NOEXEC
            sec
@rts:
            rts

; RS_HDR held to the form this loader takes (rl_head's).  OUT: as rl_head's
rl_form:
            ldx         #5                                  ; 1, 0, "o65", 0
:
            lda         RS_HDR,X
            cmp         rl_o65,X
            jne         @noexec
            dex
            bpl         :-
            lda         RS_HDR + O65_MODE + 1
            and         #$F0
            jne         @noexec
            lda         RS_HDR + O65_TBASE
            jne         @noexec
            lda         RS_HDR + O65_TBASE + 1
            cmp         #>LINKED
            jne         @noexec
            clc                                             ; (Data after text, BSS after data)
            lda         RS_HDR + O65_TBASE
            adc         RS_HDR + O65_TLEN
            tax
            lda         RS_HDR + O65_TBASE + 1
            adc         RS_HDR + O65_TLEN + 1
            cpx         RS_HDR + O65_DBASE
            jne         @noexec
            cmp         RS_HDR + O65_DBASE + 1
            jne         @noexec
            clc
            lda         RS_HDR + O65_DBASE
            adc         RS_HDR + O65_DLEN
            tax
            lda         RS_HDR + O65_DBASE + 1
            adc         RS_HDR + O65_DLEN + 1
            cpx         RS_HDR + O65_BBASE
            jne         @noexec
            cmp         RS_HDR + O65_BBASE + 1
            jne         @noexec
            lda         RS_HDR + O65_ZBASE + 1              ; (Its zero page within it)
            ora         RS_HDR + O65_ZLEN + 1
            jne         @noexec
            clc
            lda         RS_HDR + O65_ZBASE
            adc         RS_HDR + O65_ZLEN
            jcs         @noexec
            lda         R_ENTRY + 1                         ; (A program's zero page: the program's at most; a
            cmp         #RF_PROG                            ;   child's, the bytes its caller gives)
            bne         :+
            lda         RS_HDR + O65_ZLEN
            cmp         #PROG_ZP_END - PROG_ZP + 1
            jcs         @noexec
            bra         @ok
:
            lda         L_ARGLEN
            cmp         RS_HDR + O65_ZLEN
            bcs         @ok
            lda         #E_RANGE
            sec
            rts
@ok:
            clc
@rts:
            rts

@noexec:
            lda         #E_NOEXEC
            sec
            rts

; r1 bytes from L_FD to r0, all of them (RS_DST, RS_CNT: the bank in the window's scratch).  OUT: C = 1, .A = the
; error (READ's; E_NOEXEC: the file ended too soon)
rl_readx:
            lda         r0
            sta         RS_DST
            lda         r0 + 1
            sta         RS_DST + 1
            lda         r1
            sta         RS_CNT
            lda         r1 + 1
            sta         RS_CNT + 1
@more:
            lda         RS_CNT
            ora         RS_CNT + 1
            beq         @done
            lda         RS_DST
            sta         r0
            lda         RS_DST + 1
            sta         r0 + 1
            lda         RS_CNT
            sta         r1
            lda         RS_CNT + 1
            sta         r1 + 1
            lda         L_FD
            FARCALL     K_READ
            bcs         @rts
            sta         K_TMP
            stx         K_TMP2
            ora         K_TMP2
            beq         @short
            clc
            lda         RS_DST
            adc         K_TMP
            sta         RS_DST
            lda         RS_DST + 1
            adc         K_TMP2
            sta         RS_DST + 1
            sec
            lda         RS_CNT
            sbc         K_TMP
            sta         RS_CNT
            lda         RS_CNT + 1
            sbc         K_TMP2
            sta         RS_CNT + 1
            bra         @more

@done:
            clc
@rts:
            rts

@short:
            lda         #E_NOEXEC
            sec
            rts

; The file's next byte, through the read buffer (RS_BP, RS_BSZ bytes: K_PTR2 its next byte, K_CNT the bytes left in
; it, 0 to start).  OUT: .A; C = 1 at its end.  Keeps .X, .Y
rl_byte:
            lda         K_CNT
            ora         K_CNT + 1
            beq         @fill
@have:
            lda         K_CNT
            bne         :+
            dec         K_CNT + 1
:
            dec         K_CNT
            lda         (K_PTR2)
            inc         K_PTR2
            bne         :+
            inc         K_PTR2 + 1
:
            clc
            rts

@fill:
            phx                                             ; (Its next bytes: as many as it holds)
            phy
            lda         RS_BP
            sta         r0
            lda         RS_BP + 1
            sta         r0 + 1
            lda         RS_BSZ
            sta         r1
            lda         RS_BSZ + 1
            sta         r1 + 1
            lda         L_FD
            FARCALL     K_READ
            ply
            bcs         @end
            sta         K_CNT
            txa
            plx
            sta         K_CNT + 1
            ora         K_CNT
            sec
            beq         @rts
            lda         RS_BP
            sta         K_PTR2
            lda         RS_BP + 1
            sta         K_PTR2 + 1
            bra         @have

@end:
            plx
@rts:
            rts

; A relocation table applied from L_LEFT + 1 on (L_LEFT: its segment's start, less one), up to its 0: each place
; its offset from the last (255: 254 more, and on), then its type (the value's kind, the high 3 bits; its segment,
; the low 5: its text's, data's or BSS's moved by RS_DELTA, its zero page's by RS_ZPAT; an absolute one not at
; all), a HIGH's low byte after it.  OUT: C = 1, .A = E_NOEXEC: a kind this loader doesn't take (an import's)
rl_reloc:
@next:
            jsr         rl_byte
            jcs         @bad
            cmp         #0
            jeq         @done
            cmp         #255
            bne         @step
            lda         #254
            jsr         @add
            bra         @next

@step:
            jsr         @add
            jsr         rl_byte
            jcs         @bad
            sta         RS_TYPE
            and         #$1F                                ; Its segment's value
            cmp         #1
            beq         @abs
            cmp         #5
            beq         @zp
            jcs         @bad
            cmp         #2
            bcc         @imp
            lda         RS_DELTA
            sta         RS_VAL
            lda         RS_DELTA + 1
            sta         RS_VAL + 1
            bra         @kind
@imp:
            lda         RS_KIND                             ; (An import's: a child's, its index next)
            jne         @bad
            jsr         rl_byte
            jcs         @bad
            sta         RS_VAL
            jsr         rl_byte
            jcs         @bad
            sta         RS_VAL + 1
            asl         RS_VAL
            rol         RS_VAL + 1
            clc
            lda         RS_VAL
            adc         RS_IMPA
            sta         K_PTR
            lda         RS_VAL + 1
            adc         RS_IMPA + 1
            sta         K_PTR + 1
            ldy         #0
            lda         (K_PTR),Y
            sta         RS_VAL
            iny
            lda         (K_PTR),Y
            sta         RS_VAL + 1
            jmp         @kind
@zp:
            lda         RS_ZPD
            sta         RS_VAL
            stz         RS_VAL + 1
            bra         @kind
@abs:
            stz         RS_VAL
            stz         RS_VAL + 1
@kind:
            lda         RS_TYPE
            and         #$E0
            cmp         #$80                                ; WORD
            beq         @word
            cmp         #$40                                ; HIGH (its low byte next)
            beq         @high
            cmp         #$20                                ; LOW
            bne         @bad
            clc
            lda         (L_LEFT)
            adc         RS_VAL
            sta         (L_LEFT)
            jmp         @next

@word:
            clc
            lda         (L_LEFT)
            adc         RS_VAL
            sta         (L_LEFT)
            ldy         #1
            lda         (L_LEFT),Y
            adc         RS_VAL + 1
            sta         (L_LEFT),Y
            jmp         @next

@high:
            jsr         rl_byte
            bcs         @bad
            clc
            adc         RS_VAL
            lda         (L_LEFT)
            adc         RS_VAL + 1
            sta         (L_LEFT)
            jmp         @next

@done:
            clc
            rts

@bad:
            lda         #E_NOEXEC
            sec
            rts

@add:
            clc
            adc         L_LEFT
            sta         L_LEFT
            bcc         :+
            inc         L_LEFT + 1
:
            rts

; ****************************************************************************
; Children (BANK_LOAD's): a module of a caller's, its functions called by the caller itself (the bank in the window, a
; jsr, the bank put back: sdk/c/lib/bankprog.s), its imports the caller's exports

; BANK_LOAD: a child (an o65 module: sdk/c/child.cfg) in this task's code cache, loaded if it isn't (as RUN's
; programs are, several to a bank).  IN: r0 = its path; r1 = its caller's exports (NAME.exp: "BX", 1, then each export
; its name's length, its name and its address, a 0 after the last), or 0; .A = where its zero page goes; .X = the
; bytes there it may have.  OUT: C = 0, .A = its bank, .X = its directory entry (RB_DIR + 16 x .X in the bank: RE_GEN
; its load, RE_TABLE its exports' table: each its name, a 0, its address; a 0 after the last); or C = 1, .A = the
; error, .X = why (BL_*: bankprog.h's BE_*), a missing import's name in TA_SCRATCH
K_BANK_LOAD:
            sta         R_ENTRY                             ; (Its zero page, and its kind: a child)
            stx         L_ARGLEN
            stz         R_ENTRY + 1
            lda         r0
            sta         L_PATH
            lda         r0 + 1
            sta         L_PATH + 1
            lda         r1
            sta         L_ARGS
            lda         r1 + 1
            sta         L_ARGS + 1
            lda         RAM_BANK
            sta         R_RAMB
            lda         L_PATH + 1                          ; (Its path and the exports' not in the window)
            jsr         rc_window
            bcs         @fault
            lda         L_ARGS + 1
            jsr         rc_window
            bcs         @fault
            jsr         rc_stat                             ; Its file's identity: in the cache?
            bcs         @file
            lda         #0
            ldx         R_ENTRY
            jsr         rc_find
            bcc         @have
            lda         L_PATH                              ; (No: loaded)
            sta         r0
            lda         L_PATH + 1
            sta         r0 + 1
            lda         #O_READ
            FARCALL     K_OPEN
            bcs         @file
            sta         L_FD
            stz         L_TASK                              ; (Its o65 module from the file's start)
            jsr         rl_load
            php
            pha
            lda         L_FD
            FARCALL     K_CLOSE
            pla
            plp
            bcs         @why
@have:
            jsr         re_at
            jsr         re_stamp
            lda         R_RAMB
            sta         RAM_BANK
            lda         R_BANK
            ldx         R_ENT
            clc
            rts

@fault:
            lda         #E_FAULT
@file:
            ldx         #BL_FILE
            bra         @fail
@why:
            ldx         #BL_FORMAT                          ; (rl_load's error: why)
            cmp         #E_NOEXEC
            beq         @fail
            ldx         #BL_ZP
            cmp         #E_RANGE
            beq         @fail
            ldx         #BL_EXPORTS
            cmp         #E_IO
            beq         @fail
            ldx         #BL_BANK
            cmp         #E_NOMEM
            beq         @fail
            cmp         #E_TOOBIG
            beq         @fail
            ldx         #BL_IMPORT
            cmp         #E_NOENT
            bne         @other
            pha                                             ; (Its name, from the place's scratch)
            lda         RS_MISS
            sta         K_PTR
            lda         RS_MISS + 1
            sta         K_PTR + 1
            ldy         #0
:
            lda         (K_PTR),Y
            sta         TA_SCRATCH,Y
            beq         :+
            iny
            cpy         #EXIT_MSG_MAX
            bne         :-
            lda         #0
            sta         TA_SCRATCH,Y
:
            pla
            bra         @fail
@other:
            ldx         #BL_FILE
@fail:
            pha
            lda         R_RAMB
            sta         RAM_BANK
            pla
            sec
            rts

; BANK_FLUSH: every module in this task's code cache given up (but a program running), the banks left empty freed;
; .A, if not 0, the banks the cache may have from now on (RC_BANKS at first).  OUT: C = 0
K_BANK_FLUSH:
            cmp         #0
            beq         :+
            sta         TK_RMAX
:
            lda         RAM_BANK
            sta         R_RAMB
            lda         #$FF                                ; (R_ENT: the last bank kept, $FF none yet)
            sta         R_ENT
            lda         TK_RCACHE
@bank:
            sta         R_BANK
            cmp         #$FF
            beq         @end
            sta         RAM_BANK
            lda         RB_NEXT
            sta         R_MAP                               ; (The next)
            stz         R_MAP + 1                           ; (Its modules still running)
            ldx         #RB_ENTS
@ent:
            jsr         re_atx
            ldy         #RE_GEN
            lda         (K_PTR),Y
            beq         @next
            ldy         #RE_FLAGS
            lda         (K_PTR),Y
            and         #RF_BUSY
            beq         :+
            inc         R_MAP + 1
            bra         @next
:
            ldy         #RE_GEN
            lda         #0
            sta         (K_PTR),Y
@next:
            dex
            bne         @ent
            lda         R_MAP + 1
            beq         @free
            lda         R_ENT                               ; Kept: after the last kept
            cmp         #$FF
            bne         :+
            lda         R_BANK
            sta         TK_RCACHE
            bra         @kept
:
            sta         RAM_BANK
            lda         R_BANK
            sta         RB_NEXT
@kept:
            lda         R_BANK
            sta         R_ENT
            bra         @on
@free:
            lda         R_BANK                              ; Empty: given back
            ldx         #1
            FARCALL     K_BANKS_FREE
@on:
            lda         R_MAP
            bra         @bank

@end:
            lda         R_ENT                               ; (The last kept's next: none)
            cmp         #$FF
            bne         :+
            sta         TK_RCACHE
            bra         @done
:
            sta         RAM_BANK
            lda         #$FF
            sta         RB_NEXT
@done:
            lda         R_RAMB
            sta         RAM_BANK
            clc
            rts

; Its imports (the o65 list after its data: a count, then each name, zero-terminated).  A program has none.  A child's
; names go past its BSS (RS_NAMES), and each one's address, found in its caller's exports (the file at L_ARGS), into
; the table after them (RS_IMPA, 2 bytes each: $FFFF till it's found).  OUT: C = 1, .A = the error (E_NOEXEC: a
; program's import, or no room; E_IO: the exports' file not there, or not of its form; E_NOENT: an import not in it,
; RS_MISS its name)
rl_imports:
            jsr         rl_byte                             ; (Their count: 255 at most)
            jcs         @noexec
            sta         RS_NIMP
            jsr         rl_byte
            jcs         @noexec
            cmp         #0
            jne         @noexec
            lda         RS_NIMP
            bne         :+
            clc                                             ; (None)
            rts
:
            lda         RS_KIND                             ; (A program's: none)
            jne         @noexec
            clc                                             ; Their names: past the BSS
            lda         RS_END
            adc         RS_HDR + O65_BLEN
            sta         RS_NAMES
            sta         L_LEFT
            lda         RS_END + 1
            adc         RS_HDR + O65_BLEN + 1
            sta         RS_NAMES + 1
            sta         L_LEFT + 1
            ldx         RS_NIMP
@name:
            jsr         rl_byte
            jcs         @noexec
            sta         (L_LEFT)
            jsr         rl_more
            jcs         @noexec
            cmp         #0
            bne         @name
            dex
            bne         @name
            lda         L_LEFT                              ; Their addresses: none found yet
            sta         RS_IMPA
            lda         L_LEFT + 1
            sta         RS_IMPA + 1
            ldx         RS_NIMP
:
            lda         #$FF
            sta         (L_LEFT)
            jsr         rl_more
            jcs         @noexec
            lda         #$FF
            sta         (L_LEFT)
            jsr         rl_more
            jcs         @noexec
            dex
            bne         :-
            lda         L_LEFT                              ; (The exports' reader's buffer, then a name read)
            sta         RS_XB
            lda         L_LEFT + 1
            sta         RS_XB + 1
            clc
            lda         L_LEFT
            adc         #RX_BUFLEN
            sta         RS_ENAME
            lda         L_LEFT + 1
            adc         #0
            sta         RS_ENAME + 1
            clc
            lda         RS_ENAME
            adc         #RX_NAMEMAX + 1
            tax
            lda         RS_ENAME + 1
            adc         #0
            cpx         RS_LIM
            sbc         RS_LIM + 1
            bcs         @noexec
            lda         K_PTR2                              ; (The module's reader's place, kept)
            sta         RS_SAVP
            lda         K_PTR2 + 1
            sta         RS_SAVP + 1
            lda         K_CNT
            sta         RS_SAVC
            lda         K_CNT + 1
            sta         RS_SAVC + 1
            jsr         rl_resolve
            php
            pha
            lda         RS_SAVP
            sta         K_PTR2
            lda         RS_SAVP + 1
            sta         K_PTR2 + 1
            lda         RS_SAVC
            sta         K_CNT
            lda         RS_SAVC + 1
            sta         K_CNT + 1
            pla
            plp
            bcs         @rts
            lda         RS_NAMES                            ; Each found?  (K_PTR its name, L_LEFT its address)
            sta         K_PTR
            lda         RS_NAMES + 1
            sta         K_PTR + 1
            lda         RS_IMPA
            sta         L_LEFT
            lda         RS_IMPA + 1
            sta         L_LEFT + 1
            ldx         RS_NIMP
@check:
            ldy         #1
            lda         (L_LEFT),Y
            and         (L_LEFT)
            cmp         #$FF
            beq         @missing
            jsr         rl_pastname
            clc
            lda         L_LEFT
            adc         #2
            sta         L_LEFT
            bcc         :+
            inc         L_LEFT + 1
:
            dex
            bne         @check
            clc
@rts:
            rts

@missing:
            lda         K_PTR
            sta         RS_MISS
            lda         K_PTR + 1
            sta         RS_MISS + 1
            lda         #E_NOENT
            sec
            rts

@noexec:
            lda         #E_NOEXEC
            sec
            rts

; L_LEFT on one, within the module's room (RS_LIM).  OUT: C = 1 if it's reached it.  Keeps .A, .X
rl_more:
            inc         L_LEFT
            bne         :+
            inc         L_LEFT + 1
:
            pha
            lda         L_LEFT
            cmp         RS_LIM
            lda         L_LEFT + 1
            sbc         RS_LIM + 1
            pla
            rts

; K_PTR past the name it's at (its 0 too).  Keeps .X
rl_pastname:
            ldy         #0
:
            lda         (K_PTR),Y
            iny
            cmp         #0
            bne         :-
            tya
            clc
            adc         K_PTR
            sta         K_PTR
            bcc         :+
            inc         K_PTR + 1
:
            rts

; The imports' addresses from the caller's exports (the file at L_ARGS, through its own reader: rx_byte).  OUT: C = 1,
; .A = E_IO (the file not there, not of its form)
rl_resolve:
            lda         L_ARGS
            ora         L_ARGS + 1
            jeq         @io
            lda         L_ARGS
            sta         r0
            lda         L_ARGS + 1
            sta         r0 + 1
            lda         #O_READ
            FARCALL     K_OPEN
            jcs         @io
            sta         RS_XFD
            stz         RS_XC
            ldx         #0                                  ; "BX", 1
:
            jsr         rx_byte
            bcs         @bad
            cmp         rl_bx,X
            bne         @bad
            inx
            cpx         #3
            bne         :-
@export:
            jsr         rx_byte                             ; An export: its name, its address
            bcs         @bad
            cmp         #0
            beq         @done
            cmp         #RX_NAMEMAX + 1
            bcs         @bad
            sta         K_TMP2
            ldy         #0
:
            jsr         rx_byte
            bcs         @bad
            phy
            pha
            lda         RS_ENAME
            sta         K_PTR
            lda         RS_ENAME + 1
            sta         K_PTR + 1
            pla
            ply
            sta         (K_PTR),Y
            iny
            cpy         K_TMP2
            bne         :-
            lda         #0
            sta         (K_PTR),Y
            jsr         rx_byte
            bcs         @bad
            sta         RS_VAL
            jsr         rx_byte
            bcs         @bad
            sta         RS_VAL + 1
            jsr         rl_match
            bra         @export

@done:
            lda         RS_XFD
            FARCALL     K_CLOSE
            clc
            rts

@bad:
            lda         RS_XFD
            FARCALL     K_CLOSE
@io:
            lda         #E_IO
            sec
            rts

; Each import named RS_ENAME's name given the address RS_VAL (K_PTR the names, L_LEFT the addresses)
rl_match:
            lda         RS_NAMES
            sta         K_PTR
            lda         RS_NAMES + 1
            sta         K_PTR + 1
            lda         RS_IMPA
            sta         L_LEFT
            lda         RS_IMPA + 1
            sta         L_LEFT + 1
            lda         RS_ENAME
            sta         K_PTR2
            lda         RS_ENAME + 1
            sta         K_PTR2 + 1
            ldx         RS_NIMP
@imp:
            ldy         #0
@cmp:
            lda         (K_PTR),Y
            cmp         (K_PTR2),Y
            bne         @no
            iny
            cmp         #0
            bne         @cmp
            lda         RS_VAL                              ; (The same: its address)
            sta         (L_LEFT)
            ldy         #1
            lda         RS_VAL + 1
            sta         (L_LEFT),Y
@no:
            jsr         rl_pastname
            clc
            lda         L_LEFT
            adc         #2
            sta         L_LEFT
            bcc         :+
            inc         L_LEFT + 1
:
            dex
            bne         @imp
            rts

; The exports' file's next byte, through its buffer (RS_XB, RX_BUFLEN bytes: RS_XP its next, RS_XC the bytes left).
; OUT: .A; C = 1 at its end.  Keeps .X, .Y
rx_byte:
            phx
            phy
            lda         RS_XC
            bne         @have
            lda         RS_XB
            sta         r0
            sta         RS_XP
            lda         RS_XB + 1
            sta         r0 + 1
            sta         RS_XP + 1
            lda         #RX_BUFLEN
            sta         r1
            stz         r1 + 1
            lda         RS_XFD
            FARCALL     K_READ
            bcs         @end
            cmp         #0
            beq         @end
            sta         RS_XC
@have:
            lda         RS_XP
            sta         K_PTR
            lda         RS_XP + 1
            sta         K_PTR + 1
            lda         (K_PTR)
            inc         RS_XP
            bne         :+
            inc         RS_XP + 1
:
            dec         RS_XC
            ply
            plx
            clc
            rts

@end:
            ply
            plx
            sec
            rts

; A child's exports (the o65 list after its relocations: a count, then each its name, zero-terminated, its segment and
; its value) as its table, past its BSS (RS_NAMES, over its imports' names): each its name, a 0, its address (moved
; with its segment); a 0 after the last.  RS_TEND its end.  OUT: C = 1, .A = E_NOEXEC (no room)
rl_exports:
            clc                                             ; (Past the BSS: the names' place, if it had imports)
            lda         RS_END
            adc         RS_HDR + O65_BLEN
            sta         RS_NAMES
            sta         L_LEFT
            lda         RS_END + 1
            adc         RS_HDR + O65_BLEN + 1
            sta         RS_NAMES + 1
            sta         L_LEFT + 1
            jsr         rl_byte                             ; (Their count: 255 at most)
            jcs         @noexec
            tax
            jsr         rl_byte
            bcs         @noexec
            cmp         #0
            bne         @noexec
            cpx         #0
            beq         @end
@export:
            jsr         rl_byte                             ; Its name
            bcs         @noexec
            sta         (L_LEFT)
            jsr         rl_more
            bcs         @noexec
            cmp         #0
            bne         @export
            jsr         rl_byte                             ; Its segment ...
            bcs         @noexec
            sta         RS_TYPE
            jsr         rl_byte                             ; ... and value: moved with a segment of its own
            bcs         @noexec
            sta         RS_VAL
            jsr         rl_byte
            bcs         @noexec
            sta         RS_VAL + 1
            lda         RS_TYPE
            cmp         #2
            bcc         :+
            cmp         #5
            bcs         :+
            clc
            lda         RS_VAL
            adc         RS_DELTA
            sta         RS_VAL
            lda         RS_VAL + 1
            adc         RS_DELTA + 1
            sta         RS_VAL + 1
:
            lda         RS_VAL
            sta         (L_LEFT)
            jsr         rl_more
            bcs         @noexec
            lda         RS_VAL + 1
            sta         (L_LEFT)
            jsr         rl_more
            bcs         @noexec
            dex
            bne         @export
@end:
            lda         #0                                  ; (The table's end)
            sta         (L_LEFT)
            jsr         rl_more
            bcs         @noexec
            lda         L_LEFT
            sta         RS_TEND
            lda         L_LEFT + 1
            sta         RS_TEND + 1
            clc
            rts

@noexec:
            lda         #E_NOEXEC
            sec
            rts

.segment "KRODATA_P5"

rl_bx:      .byte       "BX", 1
rl_hyx2:    .byte       "HYX2"
rl_o65:     .byte       1, 0, "o65", 0
rc_idbytes: .byte       SR_QTYPE, SR_QVERS, SR_LENGTH, SR_LENGTH + 1, SR_LENGTH + 2, SR_LENGTH + 3
            .byte       SR_MTIME, SR_MTIME + 1, SR_MTIME + 2, SR_MTIME + 3, SR_DEV, SR_INST, $FF
rn_bits:    .byte       $01, $02, $04, $08, $10, $20, $40, $80
