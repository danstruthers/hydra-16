; ****************************************************************************
; fprog.inc - forth's programs: an rc command line, or a program and its arguments, started (SPAWN) and waited for.
; The Hydra library's SH and RUN (hydra.fl) and the shell's rc lines and program words (shell.fl) use it: what two
; libraries use stays in the core, headerless (included by forth.s)

; ( c-addr u -- ): argbuf rc's arguments for the command line, -c and the line (rc -c's), for rc_spawn
prog_rc:
            jsr         arg_line
            LDR         w2, argbuf
            LDR         p2, argbuf + ARGS_MAX - 2
            lda         #'-'
            jsr         arg_put
            lda         #'c'
            jsr         arg_put
            lda         #0
            jsr         arg_put
:
            lda         tmp
            ora         tmp + 1
            beq         :+
            lda         (w)
            jsr         arg_put
            jsr         arg_next
            bra         :-
:
            lda         #0                                  ; (Its 0, and the empty one after it)
            sta         (w2)
            ldy         #1
            sta         (w2),y
            rts

; ( c-addr u -- ): a program and its arguments, split at blanks (a name with no / in it is /bin's): r0 its path
; (pathbuf), argbuf its arguments (each zero-terminated, then an empty one).  No name: THROW -16
prog_args:
            jsr         arg_line
            LDR         w2, pathbuf + 5                     ; Its name (room for /bin/ before it)
            LDR         p2, pathbuf + PATH_SIZE - 1
            jsr         next_arg
            bcc         :+
            lda         #<-16
            jmp         throw_a
:
            LDR         r0, pathbuf + 5
            lda         pathbuf + 5
            cmp         #'#'
            beq         @args
            ldy         #0
:
            lda         pathbuf + 5,y
            beq         @bin
            iny
            cmp         #'/'
            bne         :-
            bra         @args
@bin:
            ldy         #4                                  ; (/bin/ before it)
:
            lda         s_binrc,y
            sta         pathbuf,y
            dey
            bpl         :-
            LDR         r0, pathbuf
@args:
            LDR         w2, argbuf
            LDR         p2, argbuf + ARGS_MAX - 1
:
            jsr         next_arg
            bcc         :-
            lda         #0
            sta         (w2)
            rts

; ( c-addr u -- ): w and tmp the string (w its next char, tmp the chars left)
arg_line:
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
            inx
            inx
            rts

; SPAWN rc with prog_rc's arguments (/bin/rc, else the ROM's, #m/rc), .A its flags (as prog_spawn's).  OUT: as
; prog_spawn's
rc_spawn:
            pha
            LDR         r0, s_binrc
            pla
            pha
            jsr         prog_spawn
            bcc         @done
            cmp         #E_NOENT
            bne         @done
            LDR         r0, s_mrc
            pla
            pha
            jsr         prog_spawn
@done:
            ply
            rts

; SPAWN r0's program with argbuf's arguments, .A its flags (SPAWN_NEWGROUP; SPAWN_FDMAP: prog_map's fds), the output
; waiting out first (r0 kept over its WRITE).  OUT: C = 0, .A the task; or C = 1, .A the error
prog_spawn:
            pha
            lda         r0
            pha
            lda         r0 + 1
            pha
            jsr         flush
            pla
            sta         r0 + 1
            pla
            sta         r0
            LDR         r1, argbuf
            LDR         r2, prog_map
            pla
            stx         xsave
            jsr         SPAWN
            ldx         xsave
            rts

; Task .A waited for (a note ending the wait: waited for again): tmp2 its exit code, or, if that's 1 and its message
; (statbuf) is a number (rc's $status), the number.  A failure: THROW
prog_wait:
            sta         tmp
@wait:
            LDR         r0, statbuf
            lda         tmp
            stx         xsave
            jsr         WAIT
            stx         tmp2
            ldx         xsave
            bcc         @ended
            cmp         #E_INTR
            beq         @wait
            jmp         throw_os
@ended:
            stz         tmp2 + 1
            lda         tmp2
            cmp         #1
            bne         @done
            lda         statbuf                             ; (A number?)
            sec
            sbc         #'0'
            cmp         #10
            bcs         @done
            stz         tmp2
            ldy         #0
@digit:
            lda         statbuf,y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @done
            pha
            asl         tmp2                                ; (* 10: * 2, kept, * 4, and the kept added)
            rol         tmp2 + 1
            lda         tmp2
            sta         numtmp
            lda         tmp2 + 1
            sta         numtmp + 1
            asl         tmp2
            rol         tmp2 + 1
            asl         tmp2
            rol         tmp2 + 1
            clc
            lda         tmp2
            adc         numtmp
            sta         tmp2
            lda         tmp2 + 1
            adc         numtmp + 1
            sta         tmp2 + 1
            clc
            pla
            adc         tmp2
            sta         tmp2
            bcc         :+
            inc         tmp2 + 1
:
            iny
            bra         @digit
@done:
            rts

; The next word of the string at w (tmp chars left) into (w2), zero-terminated, w2 past its 0.  OUT: C = 1 if there
; was none (nothing stored)
next_arg:
@skip:
            lda         tmp
            ora         tmp + 1
            beq         @none
            lda         (w)
            cmp         #' ' + 1
            bcs         @word
            jsr         arg_next
            bra         @skip
@word:
            lda         tmp
            ora         tmp + 1
            beq         @end
            lda         (w)
            cmp         #' ' + 1
            bcc         @end
            jsr         arg_put
            jsr         arg_next
            bra         @word
@end:
            lda         #0
            jsr         arg_put
            clc
            rts
@none:
            sec
            rts

; w on a char, tmp one less
arg_next:
            jsr         w_inc
            lda         tmp
            bne         :+
            dec         tmp + 1
:
            dec         tmp
            rts

; .A into (w2), w2 on; at p2 (the buffer's end): THROW the ior of E_NAMETOOLONG.  Keeps .Y
arg_put:
            pha
            lda         w2
            cmp         p2
            lda         w2 + 1
            sbc         p2 + 1
            pla
            bcc         :+
            lda         #E_NAMETOOLONG
            jmp         throw_os
:
            sta         (w2)
            inc         w2
            bne         :+
            inc         w2 + 1
:
            rts

s_binrc:    .byte       "/bin/rc", 0
s_mrc:      .byte       "#m/rc", 0
