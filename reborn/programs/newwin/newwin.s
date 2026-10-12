; ****************************************************************************
; new-window [-g] [command ...] - a window made and shown, in this one's group (-g: a group of its own, a new shell
; session; docs/design/plans/WINDOWS.md, W5), and in it the command (rc -l -c: the default namespace and the profile
; first, as a shell's start, so its /dev is the window's) or, with none, the shell (/lib/shell's line, as init and
; wstart have it; with none, rc -l).  The program run has a note group of its own, $window the window's number in its
; environment, and the window's cons as its fds 0-2; it isn't waited for, and the window goes when it ends (its last
; cons closed).  The window is made by this one's wctl's new (new group), its number its fid's next read.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "new-window", main

F_G             = $01           ; -g
SH_MAX          = 64            ; /lib/shell's bytes read, at most
CMD_MAX         = 120           ; The command's words joined, at most

.bss
fd:         .res        1                                   ; /dev/wctl
digits:     .res        1                                   ; The window's number's digits
cur:        .res        12                                  ; "current N"
at:         .res        1                                   ; (command's: its place in cmd)
buf:        .res        8                                   ; Its number, as wctl answers it
name:       .res        12                                  ; "#cN/cons"
map:        .res        4                                   ; The program's fds: the window's cons
prog:       .res        2                                   ; The program's path ...
args:       .res        2                                   ;   and its arguments (SPAWN's: each zero-ended, an empty
                                                            ;   one after the last)
shraw:      .res        SH_MAX + 2                          ; /lib/shell, its words
cmd:        .res        CMD_MAX + 8                         ; rc's: -l, -c, the command

.code
main:
            jsr         tl_start
            LDR         r0, s_wctl                          ; The window: wctl's new (new group)
            lda         #O_RDWR
            jsr         OPEN
            bcs         @wctl
            sta         fd
            LDR         r0, s_new
            LDR         r1, 3
            lda         tl_flags
            and         #F_G
            beq         :+
            LDR         r1, 9
:
            lda         fd
            jsr         WRITE
            bcs         @wctl
            stz         r0                                  ; (Its answer: from the start)
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            lda         fd
            ldx         #0
            jsr         SEEK
            LDR         r0, buf
            LDR         r1, 7
            lda         fd
            jsr         READ
            bcs         @wctl
            ldx         #0                                  ; (Its digits: one or two)
:
            lda         buf,X
            cmp         #'0'
            bcc         :+
            cmp         #'9' + 1
            bcs         :+
            inx
            cpx         #2
            bcc         :-
:
            stx         digits
            cpx         #0
            bne         :+
            lda         #E_IO
@wctl:
            LDR         r0, s_wctl
            jsr         tl_err
            jmp         tl_end
:
            ldx         #0                                  ; Shown: current N, one write
:
            lda         s_current,X
            sta         cur,X
            inx
            cpx         #8
            bcc         :-
            ldy         #0
:
            lda         buf,Y
            sta         cur,X
            inx
            iny
            cpy         digits
            bcc         :-
            stx         r1
            stz         r1 + 1
            LDR         r0, cur
            lda         fd
            jsr         WRITE
            lda         fd
            jsr         CLOSE
            LDR         r0, s_window                        ; $window: N
            LDR         r1, buf
            lda         digits
            sta         r2
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            lda         #$FF
            jsr         ENV_PUT
            lda         #'#'                                ; "#cN/cons"
            sta         name
            lda         #'c'
            sta         name + 1
            ldx         #0
:
            lda         buf,X
            sta         name + 2,X
            inx
            cpx         digits
            bcc         :-
            ldy         #0
:
            lda         s_cons,Y                            ; ("/cons", and its zero)
            sta         name + 2,X
            inx
            iny
            cpy         #6
            bcc         :-
            LDR         r0, name
            lda         #O_RDWR
            jsr         OPEN
            bcc         :+
            LDR         r0, name
            jsr         tl_err
            jmp         tl_end
:
            sta         map + 1                             ; Its fds 0-2: the window's
            sta         map + 2
            sta         map + 3
            lda         #3
            sta         map
            lda         (tl_arg)                            ; What runs: the command, or the shell
            beq         @shell
            jsr         command
            bra         @spawn
@shell:
            jsr         shell
@spawn:
            lda         prog
            sta         r0
            lda         prog + 1
            sta         r0 + 1
            lda         args
            sta         r1
            lda         args + 1
            sta         r1 + 1
            LDR         r2, map
            lda         #SPAWN_NEWGROUP | SPAWN_NEWNS | SPAWN_FDMAP
            jsr         SPAWN
            bcc         :+
            ldx         prog                                ; (.A: the error)
            stx         r0
            ldx         prog + 1
            stx         r0 + 1
            jsr         tl_err
:
            jmp         tl_end

; The command, from tl_arg on: rc -l -c and its words joined by spaces (CMD_MAX bytes at most)
command:
            LDR         prog, s_rc
            LDR         args, cmd
            ldx         #0                                  ; -l, -c
:
            lda         s_lc,X
            sta         cmd,X
            inx
            cpx         #S_LC
            bcc         :-
@word:
            ldy         #0
:
            lda         (tl_arg),Y
            beq         @next
            cpx         #S_LC + CMD_MAX
            bcs         @next
            sta         cmd,X
            inx
            iny
            bne         :-
@next:
            stx         at
            jsr         tl_next
            php
            ldx         at
            plp
            beq         @end
            cpx         #S_LC + CMD_MAX
            bcs         @end
            lda         #' '
            sta         cmd,X
            inx
            bra         @word
@end:
            stz         cmd,X                               ; (Its end, and the list's)
            stz         cmd + 1,X
            rts

; The shell: /lib/shell's first line (its program, then its arguments: a word each), else rc -l
shell:
            LDR         prog, s_rc
            LDR         args, s_l
            LDR         r0, s_lshell
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            rts
:
            sta         fd
            LDR         r0, shraw
            LDR         r1, SH_MAX
            lda         fd
            jsr         READ
            php
            pha
            lda         fd
            jsr         CLOSE
            pla
            plp
            bcs         @done
            tax                                             ; (Its end: a zero, and the list's)
            stz         shraw,X
            stz         shraw + 1,X
            ldx         #0                                  ; Its first line's words, each ended with a zero
            ldy         #0                                  ;   (.X: from, .Y: to)
@skip:
            lda         shraw,X
            beq         @eol
            cmp         #LF
            beq         @eol
            cmp         #' ' + 1
            bcs         @word
            inx
            bra         @skip
@word:
            lda         shraw,X
            cmp         #' ' + 1
            bcc         @wend
            sta         shraw,Y
            inx
            iny
            bra         @word
@wend:
            pha                                             ; (The byte it stopped at: the word's 0 may go there)
            lda         #0
            sta         shraw,Y
            iny
            pla
            beq         @eol
            cmp         #LF
            beq         @eol
            inx
            bra         @skip
@eol:
            cpy         #0
            beq         @done                               ; (None: rc -l)
            lda         #0
            sta         shraw,Y
            LDR         prog, shraw                         ; The program, its arguments after it
            ldy         #0
:
            lda         shraw,Y
            beq         :+
            iny
            bra         :-
:
            iny
            clc
            tya
            adc         #<shraw
            sta         args
            lda         #>shraw
            adc         #0
            sta         args + 1
@done:
            rts

.rodata
s_wctl:     .byte       "/dev/wctl", 0
s_new:      .byte       "new group"
s_current:  .byte       "current "
s_window:   .byte       "window", 0
s_cons:     .byte       "/cons", 0
s_rc:       .byte       "#m/rc", 0
s_l:        .byte       "-l", 0, 0
s_lc:       .byte       "-l", 0, "-c", 0
S_LC        = * - s_lc
s_lshell:   .byte       "/lib/shell", 0
tl_name:    .byte       "new-window", 0
tl_flagset: .byte       "g", 0
tl_usage:   .byte       "new-window [-g] [command ...]", 0

.include "toollib.s"
