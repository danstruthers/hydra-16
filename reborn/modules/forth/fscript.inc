; ****************************************************************************
; fscript.inc - forth's Ctrl-C and scripts (included by forth.s): Ctrl-C's THROW, and forth file.fs.  (The Hydra's
; words are a library: forthlib/hydra.s.)

; ---- Ctrl-C: the note handler (forth.s's notes) sets intr; a word interpreted, a loop (compiled: comp_poll; DO's:
; xloop, xploop) and a wait (KEY, MS, a file's line) THROW -28 then

; THROW -28, intr cleared
intr_throw:
            stz         intr
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
