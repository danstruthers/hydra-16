; ****************************************************************************
; tsh - the test shell, till rc comes (phase 3): one in each console window.  "tsh N" is window N's: its fds 0-2
; the window's cons, the window's notes its own note group's (consctl's group), and the window's console at /dev in
; its namespace.  A line at a time:
;   ps          the tasks
;   ls PATH     a directory's names (a / after each directory's)
;   cat PATH    a file
;   cd PATH     the current directory (pwd: what it is)
;   NAME ...    the module NAME (#m/NAME) run with the rest of the line as its arguments, and waited for (Ctrl-C
;               ends it: the shell's note handler keeps the shell going)
; "tsh w" starts the windows' shells: it waits for the user's Ctrl-] c (a read of #c/wnew: the window made), starts
; "tsh N" there, and ends (init starts it again; the new shell is init's to wait for then).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "tsh", main

LINE_MAX        = 127

.zeropage
args:       .res        2
child:      .res        1
fd:         .res        1
len:        .res        1
win:        .res        1                                   ; The window's digit
rec:        .res        2                                   ; A stat record, in buf
left:       .res        2                                   ; The bytes of them a read gave, still to show

.bss
msg:        .res        32                                  ; An exit message, an error's text
line:       .res        LINE_MAX + 1
buf:        .res        512
n_cons:     .res        16                                  ; "#cN/cons" ...
n_ctl:      .res        16                                  ;   "#cN/consctl" ...
n_dev:      .res        8                                   ;   and "#cN"
n_prog:     .res        20                                  ; "#m/NAME"

.code
main:
            MOVR        args, r0
            LDR         r0, notes                           ; (Notes don't end it: what it runs, they do)
            jsr         NOTIFY
            lda         (args)
            cmp         #'w'
            bne         :+
            jmp         starter
:
            sec                                             ; Its window: the argument's digit (none: 0)
            sbc         #'0'
            cmp         #10
            bcc         :+
            lda         #0
:
            ora         #'0'
            sta         win
            jsr         names
            jsr         window
            PRINT       s_hello
            lda         win
            jsr         PUTC
            PRINT       s_crlf

; ****************************************************************************
; The shell
shell:
            PRINT       s_prompt
            lda         win
            jsr         PUTC
            PRINT       s_gt
            jsr         getline
            bcs         shell
            lda         len                                 ; (An empty line: nothing)
            beq         shell
            ldx         #0                                  ; Its command
@command:
            lda         cmd_names,X
            sta         r0
            lda         cmd_names + 1,X
            sta         r0 + 1
            ora         r0
            beq         @run
            jsr         is_cmd                              ; (It keeps .X)
            beq         @builtin
            inx
            inx
            bra         @command

@builtin:
            jsr         @go
            bra         shell

@go:
            jmp         (cmd_vec,X)

@run:                                                       ; Not one of them: a module of that name
            jsr         run
            bra         shell

; NAME ...: #m/NAME run, the rest of the line its arguments; waited for, its code said if it isn't 0
run:
            ldx         #0                                  ; "#m/" and the name
:
            lda         s_mod,X
            sta         n_prog,X
            inx
            cpx         #3
            bne         :-
            ldy         #0
:
            lda         line,Y
            beq         @args
            cmp         #' '
            beq         @args
            sta         n_prog,X
            inx
            iny
            cpx         #3 + 12
            bcc         :-
@args:
            stz         n_prog,X
            stz         r1                                  ; Its arguments: after the space, or none
            stz         r1 + 1
            lda         line,Y
            beq         :+
            iny
            tya
            clc
            adc         #<line
            sta         r1
            lda         #>line
            adc         #0
            sta         r1 + 1
:
            LDR         r0, n_prog
            lda         #0
            jsr         SPAWN
            bcc         :+
            jmp         error
:
            sta         child
@wait:
            LDR         r0, msg
            lda         child
            jsr         WAIT
            bcc         :+
            cmp         #E_INTR                             ; (A note came to this task too: still waiting)
            beq         @wait
            jmp         error
:
            cpx         #0
            beq         @done
            phx
            PRINT       s_error                             ; "tsh: code $xx (its message)"
            PRINT       s_code
            pla
            jsr         PUTHEX
            PRINT       s_open
            PRINT       msg
            PRINT       s_close
@done:
            rts

; cd PATH, pwd
cd:
            LDR         r0, line + 3
            jsr         CHDIR
            bcc         :+
            jmp         error
:
            rts

pwd:
            LDR         r0, line
            jsr         GETCWD
            PRINT       line
            PRINT       s_crlf
            rts

; A line from fd 0 into line (its LF dropped, zero-terminated, len long).  OUT: C = 0; or C = 1 (a note, the end
; of the input: nothing)
getline:
            LDR         r0, line
            LDR         r1, LINE_MAX
            lda         #0
            jsr         READ
            bcs         @none
            cmp         #0
            beq         @none                               ; (The end of the input)
            tax
            lda         line - 1,X                          ; (Its LF, if it has one)
            cmp         #LF
            bne         :+
            dex
:
            stz         line,X
            stx         len
            clc
            rts

@none:
            PRINT       s_crlf
            sec
            rts

; Z = 1 if line starts with the command at r0 (zero-terminated), followed by a space or the line's end.  Keeps .X
is_cmd:
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            cmp         line,Y
            bne         @no
            iny
            bra         :-
:
            lda         line,Y
            beq         @yes
            cmp         #' '
            beq         @yes
@no:
            lda         #1
            rts

@yes:
            lda         #0
            rts

; ls PATH: the names in a directory (its stat records), one a line
ls:
            LDR         r0, line + 3
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         error
:
            sta         fd
@read:
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            bcs         @end
            sta         left
            stx         left + 1
            ora         left + 1
            beq         @end
            LDR         rec, buf
@record:
            lda         left + 1                            ; A whole record left?
            bne         :+
            lda         left
            cmp         #SR_SIZE
            bcc         @read
:
            MOVR        r0, rec                             ; Its name (SR_NAME: 0) ...
            jsr         PUTS
            ldy         #SR_QTYPE                           ;   a / if it's a directory's
            lda         (rec),Y
            and         #QT_DIR
            beq         :+
            lda         #'/'
            jsr         PUTC
:
            PRINT       s_crlf
            clc
            lda         rec
            adc         #SR_SIZE
            sta         rec
            bcc         :+
            inc         rec + 1
:
            sec
            lda         left
            sbc         #SR_SIZE
            sta         left
            bcs         @record
            dec         left + 1
            bra         @record

@end:
            lda         fd
            jmp         CLOSE

; cat PATH: a file, to stdout
cat:
            LDR         r0, line + 4
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         error
:
            sta         fd
@read:
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            bcs         @end
            sta         r1
            stx         r1 + 1
            ora         r1 + 1
            beq         @end
            LDR         r0, buf
            lda         #1
            jsr         WRITE
            bra         @read

@end:
            lda         fd
            jmp         CLOSE

; ****************************************************************************
; The window

; Its files' names: #cN/cons, #cN/consctl, #cN
names:
            ldx         #0
:
            lda         s_cpre,X                            ; "#c"
            sta         n_cons,X
            sta         n_ctl,X
            sta         n_dev,X
            inx
            cpx         #2
            bne         :-
            lda         win
            sta         n_cons,X
            sta         n_ctl,X
            sta         n_dev,X
            stz         n_dev + 3
            ldy         #0
:
            lda         s_consctl,Y                         ; "/cons", "/consctl"
            sta         n_ctl + 3,Y
            sta         n_cons + 3,Y
            iny
            cmp         #0
            bne         :-
            lda         #0                                  ; ("/cons": "/consctl" cut short)
            sta         n_cons + 3 + 5
            rts

; Its fds 0-2 the window's cons, its notes its own note group's, and its console at /dev (but window 0's, which is
; there already).  A window that isn't there: the end
window:
            lda         #0
            jsr         CLOSE
            lda         #1
            jsr         CLOSE
            lda         #2
            jsr         CLOSE
            LDR         r0, n_cons
            lda         #O_RDWR
            jsr         OPEN
            bcc         :+
            LDR         r0, 0
            jmp         EXITS                               ; (With the error as its code)
:
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP
            LDR         r0, n_ctl                           ; The window's notes: this task's group's
            lda         #O_WRITE
            jsr         OPEN
            bcs         :+
            sta         fd
            LDR         r0, s_group
            LDR         r1, 5
            lda         fd
            jsr         WRITE
            lda         fd
            jsr         CLOSE
:
            lda         win
            cmp         #'0'
            beq         @done
            LDR         r0, s_hcons                         ; /dev: this window's console in place of window 0's
            LDR         r1, s_dev
            jsr         UNMOUNT
            LDR         r0, n_dev
            LDR         r1, s_dev
            lda         #MAFTER
            jsr         BIND
            bcc         @done
            jsr         error
@done:
            rts

; tsh w: wait for the user's Ctrl-] c, start a shell in the window made, and end
starter:
            LDR         r0, s_wnew
            lda         #O_READ
            jsr         OPEN
            bcs         @end
            sta         fd
            LDR         r0, buf
            LDR         r1, 4
            lda         fd
            jsr         READ
            bcs         @end
            lda         fd
            jsr         CLOSE
            stz         buf + 1                             ; ("N": its argument)
            LDR         r0, s_tsh
            LDR         r1, buf
            lda         #SPAWN_NEWGROUP
            jsr         SPAWN
            lda         #0
@end:
            stz         r0
            stz         r0 + 1
            jmp         EXITS

; The error .A, said: "tsh: its text"
error:
            pha
            PRINT       s_error
            LDR         r0, msg
            pla
            jsr         ERRSTR
            PRINT       msg
            PRINT       s_crlf
            rts

; The note handler: the shell goes on, whatever the note (but a kill, which isn't caught)
notes:
            clc
            rts

.rodata
s_hello:    .byte       "tsh: window ", 0
s_prompt:   .byte       "tsh ", 0
s_gt:       .byte       "> ", 0
s_crlf:     .byte       CR, LF, 0
s_error:    .byte       "tsh: ", 0
s_code:     .byte       "code $", 0
s_open:     .byte       " (", 0
s_close:    .byte       ")", CR, LF, 0
s_mod:      .byte       "#m/"
s_cpre:     .byte       "#c"
s_consctl:  .byte       "/consctl", 0
s_group:    .byte       "group"
s_hcons:    .byte       "#c", 0
s_dev:      .byte       "/dev", 0
s_wnew:     .byte       "#c/wnew", 0
s_tsh:      .byte       "#m/tsh", 0
s_ps:       .byte       "ps", 0
s_ls:       .byte       "ls", 0
s_cat:      .byte       "cat", 0
s_cd:       .byte       "cd", 0
s_pwd:      .byte       "pwd", 0
cmd_names:  .word       s_ps, s_ls, s_cat, s_cd, s_pwd, 0
cmd_vec:    .word       DBG_PS, ls, cat, cd, pwd
