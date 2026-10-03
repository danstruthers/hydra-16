; ****************************************************************************
; init - the first program (task 1), for phase 2: its fds 0-2 on the console (#c/cons, the console driver's; or,
; without it, none, and the bring-up console's); its namespace, built in till there are disks (the plan's appendix
; E, as far as the devices there are go: #/ at /, #c, #n and #t at /dev, #m at /dev/mod, #p at /proc); the tasks
; listed; hello run and waited for; then a test shell till rc comes (phase 3), a line at a time:
;   ps          the tasks
;   ls PATH     a directory's names (a / after each directory's)
;   cat PATH    a file
;   cd PATH     the current directory (pwd: what it is)
;   anything else comes back with a ?
; Its note handler keeps it going (Ctrl-C is the foreground group's, and init's group is the foreground at first).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "init", main

LINE_MAX        = 127

.zeropage
child:      .res        1
fd:         .res        1
len:        .res        1
rec:        .res        2                                   ; A stat record, in buf
left:       .res        2                                   ; The bytes of them a read gave, still to show

.bss
msg:        .res        32                                  ; An exit message, an error's text
line:       .res        LINE_MAX + 1
buf:        .res        512

.code
main:
            LDR         r0, notes                           ; (Notes don't end it)
            jsr         NOTIFY
            LDR         r0, s_cons                          ; Fds 0-2: the console
            lda         #O_RDWR
            jsr         OPEN
            bcs         :+                                  ; (No console driver: the bring-up console's)
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP
:
            jsr         namespace
            PRINT       s_up
            jsr         GETPID
            jsr         PUTHEX
            PRINT       s_crlf
            jsr         DBG_PS
            LDR         r0, s_hello                         ; hello, with arguments, and its end
            LDR         r1, s_args
            lda         #0
            jsr         SPAWN
            bcs         @failed
            sta         child
            LDR         r0, msg
            lda         child
            jsr         WAIT
            bcs         @failed
            phx
            PRINT       s_ended                             ; hello ended: code $07 ("bye")
            pla
            jsr         PUTHEX
            PRINT       s_open
            PRINT       msg
            PRINT       s_close
            bra         shell

@failed:
            jsr         error

; ****************************************************************************
; The test shell
shell:
            PRINT       s_prompt
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
            beq         @what
            jsr         is_cmd                              ; (It keeps .X)
            beq         @run
            inx
            inx
            bra         @command

@run:
            jsr         @go
            bra         shell

@go:
            jmp         (cmd_vec,X)

@what:
            PRINT       line                                ; Not one of them
            PRINT       s_what
            bra         shell

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

; A line from fd 0 into line (its LF dropped, zero-terminated, len long); or, with no fd 0, from the bring-up
; console, a key at a time, echoed.  OUT: C = 0; or C = 1 (a note, the end of the input: nothing)
getline:
            LDR         r0, line
            LDR         r1, LINE_MAX
            lda         #0
            jsr         READ
            bcs         @polled
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

@polled:
            cmp         #E_BADF
            bne         @none
            stz         len
@key:
            jsr         GETC
            bcs         @none
            cmp         #CR
            beq         @end
            ldx         len
            cpx         #LINE_MAX
            bcs         @key
            sta         line,X
            inc         len
            jsr         PUTC
            bra         @key

@end:
            PRINT       s_crlf
            ldx         len
            stz         line,X
            clc
            rts

@none:
            PRINT       s_crlf
            sec
            rts

; Z = 1 if line starts with the command at r0 (zero-terminated), followed by a space or the line's end
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

; The namespace, built in (a bind each: its flags, new, old): what can't be bound is said, and the rest goes on
namespace:
            ldx         #0
@entry:
            lda         ns_table,X
            cmp         #$FF
            beq         @done
            pha
            lda         ns_table + 1,X
            sta         r0
            lda         ns_table + 2,X
            sta         r0 + 1
            lda         ns_table + 3,X
            sta         r1
            lda         ns_table + 4,X
            sta         r1 + 1
            pla
            phx
            jsr         BIND
            bcc         :+
            jsr         error
:
            pla
            clc
            adc         #5
            tax
            bra         @entry

@done:
            rts

; The error .A, said: "init: its text"
error:
            pha
            PRINT       s_error
            LDR         r0, msg
            pla
            jsr         ERRSTR
            PRINT       msg
            PRINT       s_crlf
            rts

; The note handler: init goes on, whatever the note (but a kill, which isn't caught)
notes:
            clc
            rts

.rodata
s_cons:     .byte       "#c/cons", 0
s_up:       .byte       "init: up in task ", 0
s_hello:    .byte       "#m/hello", 0
s_args:     .byte       "from init", 0
s_ended:    .byte       "init: hello ended: code $", 0
s_open:     .byte       " (", 0
s_close:    .byte       ")"
s_crlf:     .byte       CR, LF, 0
s_error:    .byte       "init: ", 0
s_prompt:   .byte       "init> ", 0
s_what:     .byte       "?", CR, LF, 0
s_ps:       .byte       "ps", 0
s_ls:       .byte       "ls", 0
s_cat:      .byte       "cat", 0
s_cd:       .byte       "cd", 0
s_pwd:      .byte       "pwd", 0
cmd_names:  .word       s_ps, s_ls, s_cat, s_cd, s_pwd, 0
cmd_vec:    .word       DBG_PS, ls, cat, cd, pwd
ns_table:   .byte       MREPL                               ; bind '#/' /
            .word       s_hroot, s_root
            .byte       MAFTER                              ; bind -a '#c' /dev
            .word       s_hcons, s_dev
            .byte       MAFTER                              ; bind -a '#n' /dev
            .word       s_hnull, s_dev
            .byte       MAFTER                              ; bind -a '#t' /dev
            .word       s_htime, s_dev
            .byte       MREPL                               ; bind '#m' /dev/mod
            .word       s_hmod, s_devmod
            .byte       MREPL                               ; bind '#p' /proc
            .word       s_hproc, s_proc
            .byte       $FF
s_hroot:    .byte       "#/", 0
s_hcons:    .byte       "#c", 0
s_hnull:    .byte       "#n", 0
s_htime:    .byte       "#t", 0
s_hmod:     .byte       "#m", 0
s_hproc:    .byte       "#p", 0
s_root:     .byte       "/", 0
s_dev:      .byte       "/dev", 0
s_devmod:   .byte       "/dev/mod", 0
s_proc:     .byte       "/proc", 0
