; ****************************************************************************
; fscript.inc - forth's Ctrl-C and scripts (included by forth.s): Ctrl-C's THROW, and forth file.fs.  (The Hydra's
; words are a library: forthlib/hydra.s.)

; ---- Notes: the note handler (forth.s's notes) sets intr: $C0, Ctrl-C's; $80, a note for a program's handler
; (on-note's: note_pend).  A word interpreted and a loop's step (compiled: comp_poll; DO's: xloop, xploop) take it
; (intr_check): Ctrl-C a THROW -28, and another the handler's.  A wait it ends (KEY, MS, a file's line: intr_wait)
; THROWs -28 for Ctrl-C; for another it waits again (or ends: MS), and the next word or step takes it

; The note waiting (intr): Ctrl-C's, THROW -28; a program's, its handler run (note_xt ( n -- flag ): true, forth goes
; on, and this returns; false, THROW -28 as for Ctrl-C).  A handler no longer in the dictionary: as Ctrl-C.  Keeps .X
intr_check:
            bit         intr
            bvs         intr_throw
            lda         note_xt + 1                         ; (Its handler: the core's, or in the dictionary)
            beq         intr_throw
            cmp         #$A0
            bcs         :+
            lda         note_xt
            cmp         here
            lda         note_xt + 1
            sbc         here + 1
            bcs         intr_throw
:
            lda         note_pend
            stz         note_pend
            stz         intr
            dex
            sta         dlo,x
            stz         dhi,x
            lda         note_xt
            sta         w2
            lda         note_xt + 1
            sta         w2 + 1
            jsr         exec_w2
            lda         dlo,x
            ora         dhi,x
            php
            inx
            plp
            beq         intr_throw
            rts

; A wait a note ended (E_INTR): Ctrl-C's, THROW -28; a program's, left to the next word or loop step (the caller waits
; again).  Keeps .X
intr_wait:
            bit         intr
            bvs         intr_throw
            rts

; THROW -28, intr cleared
intr_throw:
            stz         intr
            stz         note_pend
            lda         #<-28
            jmp         throw_a

; ---- Scripts

; forth file.fs [argument ...]: the file INCLUDED (a name with no / not there: /lib/forth's), then the end: code 0,
; or 1 after an error's message (its file and line)
run_script:
            lda         argp
            ldy         argp + 1
            PUSHAY
            jsr         zcount
            lda         #<included
            ldy         #>included
            PUSHAY
            jsr         catch
            lda         dlo,x
            ora         dhi,x
            beq         :+
            jsr         show_error
            lda         #1
:
            pha
            jsr         blk_end
            jsr         flush
            stz         r0
            stz         r0 + 1
            pla
            jmp         EXITS

; ( z-addr -- c-addr u ): ZCOUNT's (the Hydra library's header: hydra.fl), run_script's
zcount:
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            stz         tmp
            stz         tmp + 1
:
            lda         (w)
            beq         :+
            jsr         w_inc
            inc         tmp
            bne         :-
            inc         tmp + 1
            bra         :-
:
            lda         tmp
            ldy         tmp + 1
            PUSHAY
            rts
