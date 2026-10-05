; ****************************************************************************
; shell.s - HyForth as a shell (/lib/forth/shell.fl: lib shell; forth -l's profile.fs loads it), docs/hyforth.md.
; At the console's prompt a line is Forth if its first word is a word (found in the search order) or a number, and
; an rc command line otherwise, run whole by rc (rc -c) and waited for, its code status; one ending in & isn't
; waited for (its task: $apid).  So pipes, redirections, globbing, quoting and $x are rc's.  Only a line typed at
; the prompt goes by the rule: not one while a definition's being compiled, nor a file's or an EVALUATE's.
;   What an rc line can't do to forth (it runs in a task of its own: forth's current directory and namespace stay
; as they were) are words here, parsing their arguments as rc's built-ins do ('...' quotes, '' in it a '; $name the
; environment's variable, its first word): cd, bind, mount, unmount, newns.  And the prompt (a format: prompt, and
; prompt2 while a definition's compiled), % (the rest of a line rc's, whatever its first word), status, exits, wait,
; exit (typed at the prompt: forth ends, as rc's exit; in a definition it's Forth's), getenv and setenv, send; and
; programs as values, hylang's names: spawn, sh-out, output-of, | and piped (a word's output a program's input).
; The core finds the shell by name ((shell-prompt) and (shell-line)), so -lib shell, or a MARKER that takes it out,
; makes forth a plain Forth again; and it has the code that starts programs and waits for them (fprog.inc), which
; the Hydra library's sh and run use too.

.include "forthlib.inc"

PROMPT_MAX  = 31                                            ; A prompt's format, at most

.bss
pfmt:       .res        PROMPT_MAX + 1                      ; The prompt's format (counted) ...
pfmt2:      .res        PROMPT_MAX + 1                      ;   and the second's (a definition being compiled)
status_v:   .res        2                                   ; The last command's code (status)
pword:      .res        PATH_MAX + 1                        ; A word parsed (zero-terminated) ...
pword2:     .res        PATH_MAX + 1                        ;   the next ...
pword3:     .res        PATH_MAX + 1                        ;   and the one after (mount's spec)
cwdbuf:     .res        PATH_MAX + 1                        ; The prompt's current directory
envbuf:     .res        128                                 ; getenv's value, setenv's (and its 0)
numbuf:     .res        8                                   ; A number's text, for the environment
sw_left:    .res        1                                   ; sh_word: the line's bytes left, and where it got to
sw_len:     .res        1                                   ;   (the word's length so far)
bg:         .res        1                                   ; <> 0: the rc line ends in & (not waited for)
fl:         .res        1                                   ; bind's and mount's flags
pfd_r:      .res        1                                   ; A pipe's reading end (sh-out, piped) ...
pfd_w:      .res        1                                   ;   and its writing end
cap_s:      .res        2                                   ; What's taken (sh-out, output-of): where it starts ...
cap_p:      .res        2                                   ;   where the next byte goes ...
cap_full:   .res        1                                   ;   <> 0: no room (sh-out's: the rest read and dropped)
.code

; Its start: the prompt the old HyForth's (%v%d> : 0:/games> , /ram> ), the second rc's (a tab), status 0
lib_init:
            ldy         #S_PROMPT_LEN
:
            lda         s_prompt,y
            sta         pfmt,y
            dey
            bpl         :-
            lda         #1
            sta         pfmt2
            lda         #TAB
            sta         pfmt2 + 1
            stz         status_v
            stz         status_v + 1
            rts

s_prompt:   .byte       S_PROMPT_LEN, "%v%d> "
S_PROMPT_LEN = * - s_prompt - 1

; ---- The core's hooks

            HEADER      "(shell-prompt)", 0
shprompt:                                                   ; ( -- ): the prompt (prompt's format): %v the card (0:
            lda         state                               ;   under /sd/0), %d the directory on it (or the whole
            ora         state + 1                           ;   path), %p the whole path, %t the task, %w the window,
            beq         @first                              ;   %% a %; on a line of its own (forth's output since the
            LDR         p2, pfmt2                           ;   last may not have ended its line).  While a definition
            bra         @show                               ;   is being compiled, the second (prompt2's)
@first:
            lda         lastc
            cmp         #LF
            beq         :+
            jsr         cr
:
            LDR         p2, pfmt
@show:
            jsr         @fmt
            lda         #LF                                 ; (The line typed after it ends with the console's new
            sta         lastc                               ;   line)
            rts
@fmt:                                                       ; (The format at p2, counted)
            ldy         #0
@char:
            tya
            cmp         (p2)
            beq         @done
            iny
            lda         (p2),y
            cmp         #'%'
            beq         @pct
            jsr         emit_a
            bra         @char
@pct:
            tya
            cmp         (p2)
            beq         @done
            iny
            lda         (p2),y
            phy
            jsr         prompt_item
            ply
            bra         @char
@done:
            rts

; Prompt's %.A out
prompt_item:
            cmp         #'p'
            beq         @path
            cmp         #'d'
            beq         @dir
            cmp         #'v'
            beq         @vol
            cmp         #'t'
            beq         @task
            cmp         #'w'
            beq         @window
            cmp         #'%'
            beq         @out
            pha                                             ; (Another: as it is)
            lda         #'%'
            jsr         emit_a
            pla
@out:
            jmp         emit_a
@path:
            jsr         get_cwd
            ldy         #0
            jmp         cwd_out
@dir:
            jsr         get_cwd
            jsr         on_card
            ldy         #0
            bcs         :+
            ldy         #5                                  ; (/sd/N: the rest, or / if there's none)
            lda         cwdbuf,y
            bne         :+
            lda         #'/'
            jmp         emit_a
:
            jmp         cwd_out
@vol:
            jsr         get_cwd
            jsr         on_card
            bcs         :+
            lda         cwdbuf + 4
            jsr         emit_a
            lda         #':'
            jmp         emit_a
:
            rts
@task:
            stx         xsave
            jsr         GETPID
            ldx         xsave
            and         #$0F
            ora         #'0'
            cmp         #'9' + 1
            bcc         @out
            adc         #'A' - '9' - 2                      ; (C = 1)
            bra         @out
@window:
            LDR         r0, s_window
            LDR         r1, cwdbuf
            jsr         env_word
            ldy         #0
; cwdbuf out from .Y, to its 0
cwd_out:
            lda         cwdbuf,y
            beq         :+
            jsr         emit_a
            iny
            bra         cwd_out
:
            rts

; cwdbuf: the current directory (empty, if it can't be had)
get_cwd:
            LDR         r0, cwdbuf
            stx         xsave
            jsr         GETCWD
            ldx         xsave
            bcc         :+
            stz         cwdbuf
:
            rts

; Is cwdbuf on a card: /sd/N (N a hex digit), alone or with /... after it?  OUT: C = 0 yes
on_card:
            ldy         #3
:
            lda         cwdbuf,y
            cmp         s_sd,y
            bne         @no
            dey
            bpl         :-
            lda         cwdbuf + 4                          ; (0-9, a-f)
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         :+
            cmp         #'a'
            bcc         @no
            cmp         #'f' + 1
            bcs         @no
:
            lda         cwdbuf + 5
            beq         @yes
            cmp         #'/'
            bne         @no
@yes:
            clc
            rts
@no:
            sec
            rts

s_sd:       .byte       "/sd/"
s_window:   .byte       "window", 0

            HEADER      "(shell-line)", 0
shline:                                                     ; ( -- flag ): the line typed: true, an rc command line,
            lda         to_in + 1                           ;   run (its first word neither a word nor a number);
            pha                                             ;   else false, >IN as it was, for forth's interpreter
            lda         to_in
            pha
            jsr         parse_name
            lda         dlo,x
            ora         dhi,x
            beq         @forth2                             ; (Nothing: forth's)
            jsr         find_name
            bcc         @forth2
            jsr         number                              ; ( c-addr u -- n 1 | d 2 | 0 )
            lda         dlo,x
            beq         @rc
            cmp         #1                                  ; (A number: n 1, or d 2, dropped)
            beq         :+
            inx
:
            inx
            inx
            bra         @forth
@forth2:
            inx
            inx
@forth:
            pla
            sta         to_in
            pla
            sta         to_in + 1
            dex
            jmp         zero_tos
@rc:
            inx
            pla                                             ; (>IN as it was: the line from its start)
            sta         to_in
            pla
            sta         to_in + 1
            jsr         rc_rest
            dex
            jmp         true_tos

            HEADER      "%", 0
percent:                                                    ; ( "line" -- ): the rest of the line rc's, whatever its
            jmp         rc_rest                             ;   first word (% free: a program a word's name shadows),
                                                            ;   as a line typed at the prompt is when it's rc's

; The rest of the source's line (from >IN; >IN at its end) run by rc (prog_rc: rc -c), waited for (its code status,
; and $status), or with an & at its end not waited for ($apid its task, in a note group of its own, as rc's)
rc_rest:
            jsr         rest_line
            lda         dhi,x                               ; (Too long for rc's arguments)
            bne         :+
            lda         dlo,x
            cmp         #ARGS_MAX - 4
            bcc         :++
:
            lda         #E_NAMETOOLONG
            jmp         throw_os
:
            lda         dlo + 1,x
            sta         p1
            lda         dhi + 1,x
            sta         p1 + 1
            ldy         dlo,x
            stz         bg
@trail:                                                     ; Its end: blanks left off, and an & (not &&)
            dey
            cpy         #$FF
            beq         @args
            lda         (p1),y
            cmp         #' ' + 1
            bcc         @trail
            cmp         #'&'
            bne         @end
            tya
            beq         :+
            dey
            lda         (p1),y
            iny
            cmp         #'&'
            beq         @end
:
            inc         bg
            dey
@end:
            iny                                             ; (.Y: its length)
@args:
            cpy         #$FF
            bne         :+
            ldy         #0
:
            tya
            sta         dlo,x
            jsr         prog_rc
            lda         #0
            ldy         bg
            beq         :+
            lda         #SPAWN_NEWGROUP
:
            jsr         rc_spawn
            bcc         :+
            jmp         throw_os
:
            ldy         bg
            beq         @wait
            jsr         set_apid                            ; ($apid: its task; status 0)
            stz         tmp2
            stz         tmp2 + 1
            stz         statbuf
            jmp         set_status
@wait:
            jsr         prog_wait
            bit         intr                                ; (A Ctrl-C ended it: the shell goes on, on a new
            bvc         :+                                  ;   line, as rc does)
            stz         intr
            jsr         cr
:
            jmp         set_status

; ( -- c-addr u ): the rest of the source's line, from >IN; >IN at its end
rest_line:
            clc
            lda         src_addr
            adc         to_in
            pha
            lda         src_addr + 1
            adc         to_in + 1
            tay
            pla
            PUSHAY
            sec
            lda         src_len
            sbc         to_in
            pha
            lda         src_len + 1
            sbc         to_in + 1
            tay
            pla
            PUSHAY
            lda         src_len
            sta         to_in
            lda         src_len + 1
            sta         to_in + 1
            rts

; status = tmp2 (the code), and $status rc's: statbuf (prog_wait's: the exit's message, interrupt ...), or if it's
; empty the code
set_status:
            lda         tmp2
            sta         status_v
            ldy         tmp2 + 1
            sty         status_v + 1
            LDR         r1, statbuf
            lda         statbuf
            bne         :+
            lda         tmp2
            jsr         num_text_ay
            LDR         r1, numbuf
:
            LDR         r0, s_status
; The environment's variable r0 = r1 (zero-terminated: its text, and the 0, one word, rc's way).  A failure: no
; matter
env_put:
            ldy         #0
:
            lda         (r1),y
            beq         :+
            iny
            bra         :-
:
            iny                                             ; (Its 0 too)
            sty         r2
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            lda         #$FF
            stx         xsave
            jsr         ENV_PUT
            ldx         xsave
            rts

; $apid = task .A
set_apid:
            jsr         num_text
            LDR         r0, s_apid
            LDR         r1, numbuf
            jmp         env_put

; numbuf: .A (a task) in decimal (num_text), or .A/.Y (num_text_ay), zero-terminated
num_text:
            ldy         #0
num_text_ay:
            PUSHAY
            jsr         u_text                              ; ( u -- c-addr u )
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            inx
            inx
            sta         tmp
            ldy         #0
:
            cpy         tmp
            beq         :+
            lda         (w),y
            sta         numbuf,y
            iny
            bra         :-
:
            lda         #0
            sta         numbuf,y
            rts

s_apid:     .byte       "apid", 0
s_status:   .byte       "status", 0
s_home:     .byte       "home", 0

; ---- Programs as values (hylang's names): a program started, not waited for; a command line's output, or a word's,
; a string; a word's output a command line's input

            HEADER      "spawn", 0
spawn:                                                      ; ( c-addr u -- task ): a program and its arguments, as
            jsr         prog_args                           ;   run's, not waited for: a note group of its own, and
            lda         #SPAWN_NEWGROUP                     ;   $apid its task (wait waits for it)
            jsr         prog_spawn
            bcc         :+
            jmp         throw_os
:
            pha
            jsr         set_apid
            pla
            ldy         #0
            PUSHAY
            rts

            HEADER      "sh-out", 0
shout:                                                      ; ( c-addr u -- c-addr2 u2 ): the command line's output (rc
            jsr         prog_rc                             ;   -c's, as sh's), a string, waited for, its code
            jsr         make_pipe                           ;   status: in the dictionary's free space (gone with what
            lda         #3                                  ;   adds to the dictionary), as much as fits there
            sta         prog_map                            ; (Its fds: 0 and 2 forth's, 1 the pipe's writing end)
            stz         prog_map + 1
            lda         pfd_w
            sta         prog_map + 2
            lda         #2
            sta         prog_map + 3
            lda         #SPAWN_FDMAP
            jsr         rc_spawn
            php
            pha
            lda         pfd_w                               ; (forth's own writing end: the child has its)
            jsr         close_fd
            pla
            plp
            bcc         :+
            pha
            lda         pfd_r
            jsr         close_fd
            pla
            jmp         throw_os
:
            pha                                             ; (Its task)
            jsr         cap_start
@read:
            stz         cap_full                            ; Its output, as much as there's room for (the rest read,
            lda         cap_p                               ;   and dropped, so it can go on to its end)
            sta         r0
            lda         cap_p + 1
            sta         r0 + 1
            sec
            lda         #<DICT_END
            sbc         cap_p
            sta         r1
            lda         #>DICT_END
            sbc         cap_p + 1
            sta         r1 + 1
            bcc         @full                               ; (Past it: none)
            ora         r1
            bne         :+
@full:
            inc         cap_full
            LDR         r0, pad
            LDR         r1, PAD_SIZE
:
            lda         pfd_r
            stx         xsave
            jsr         READ
            stx         tmp + 1
            ldx         xsave
            bcc         :+
            cmp         #E_INTR                             ; (A note: on, till its end)
            beq         @read
            bra         @end
:
            sta         tmp
            ora         tmp + 1
            beq         @end
            lda         cap_full                            ; (Into the dictionary's space: kept)
            bne         @read
            clc
            lda         cap_p
            adc         tmp
            sta         cap_p
            lda         cap_p + 1
            adc         tmp + 1
            sta         cap_p + 1
            bra         @read
@end:
            lda         pfd_r
            jsr         close_fd
            pla
            jsr         prog_wait
            jsr         set_status
            jmp         cap_push

            HEADER      "output-of", 0
outputof:                                                   ; ( xt -- c-addr u ): what xt writes (forth's output, not
            jsr         flush                               ;   a program's), a string, as sh-out's (xt mustn't add to
            jsr         cap_start                           ;   the dictionary meanwhile); an error THROWn again
            lda         lastc                               ; (The console's last character, as it was after: what's
            pha                                             ;   taken doesn't go there)
            lda         #<cap_hook
            sta         out_hook
            lda         #>cap_hook
            sta         out_hook + 1
            jsr         catch
            jsr         flush
            stz         out_hook
            stz         out_hook + 1
            pla
            sta         lastc
            jsr         throw
; ( -- c-addr u ): the string taken (cap_s to cap_p)
cap_push:
            lda         cap_s
            ldy         cap_s + 1
            PUSHAY
            sec
            lda         cap_p
            sbc         cap_s
            pha
            lda         cap_p + 1
            sbc         cap_s + 1
            tay
            pla
            PUSHAY
            rts

; cap_s and cap_p: where what's taken goes, 256 bytes past HERE (the dictionary's free space, to DICT_END)
cap_start:
            clc
            lda         here
            sta         cap_s
            sta         cap_p
            lda         here + 1
            adc         #1
            sta         cap_s + 1
            sta         cap_p + 1
            rts

; flush's hook while output-of takes a word's output: obuf's .A bytes to cap_p, as there's room.  Uses only r0 and r1
; (flush comes in the middle of any word)
cap_hook:
            sta         r1
            lda         cap_p
            sta         r0
            lda         cap_p + 1
            sta         r0 + 1
            ldy         #0
@byte:
            cpy         r1
            beq         @done
            lda         r0 + 1                              ; (Room: below DICT_END)
            cmp         #>DICT_END
            bcs         @done
            lda         obuf,y
            sta         (r0)
            inc         r0
            bne         :+
            inc         r0 + 1
:
            iny
            bra         @byte
@done:
            lda         r0
            sta         cap_p
            lda         r0 + 1
            sta         cap_p + 1
            rts

            HEADER      "|", 0
bar:                                                        ; ( xt "command" -- ): the rest of the line rc's, and what
            jsr         rest_line                           ;   xt writes its input (piped's): ' words | wc -l
            jmp         piped

            HEADER      "piped", 0
piped:                                                      ; ( xt c-addr u -- ): what xt writes (forth's output) the
            jsr         prog_rc                             ;   command line's input (rc -c's), waited for, its code
            jsr         make_pipe                           ;   status; an error xt THROWs THROWn again after
            lda         #3                                  ; (Its fds: 0 the pipe's reading end, 1 and 2 forth's)
            sta         prog_map
            lda         pfd_r
            sta         prog_map + 1
            lda         #1
            sta         prog_map + 2
            lda         #2
            sta         prog_map + 3
            lda         #SPAWN_FDMAP
            jsr         rc_spawn
            php
            pha
            lda         pfd_r                               ; (forth's own reading end: the child has its)
            jsr         close_fd
            pla
            plp
            bcc         :+
            pha
            lda         pfd_w
            jsr         close_fd
            pla
            jmp         throw_os
:
            pha                                             ; (Its task)
            jsr         flush                               ; xt, its output into the pipe
            lda         lastc                               ; (The console's last character, as it was after)
            pha
            lda         #<pipe_hook
            sta         out_hook
            lda         #>pipe_hook
            sta         out_hook + 1
            jsr         catch
            jsr         flush
            stz         out_hook
            stz         out_hook + 1
            pla
            sta         lastc
            lda         pfd_w                               ; (Its end, for the reader)
            jsr         close_fd
            pla
            jsr         prog_wait
            jsr         set_status
            jmp         throw

; flush's hook while piped gives a word's output to a program: obuf's .A bytes into the pipe (no reader left: no
; matter).  Uses only r0 and r1
pipe_hook:
            sta         r1
            stz         r1 + 1
            LDR         r0, obuf
            lda         pfd_w
            jmp         WRITE

; pfd_r and pfd_w: a pipe's ends (PIPE).  A failure: THROW
make_pipe:
            stx         xsave
            jsr         PIPE
            stx         pfd_w
            ldx         xsave
            bcc         :+
            jmp         throw_os
:
            sta         pfd_r
            rts

; CLOSE fd .A (a failure: no matter).  Keeps .X
close_fd:
            stx         xsave
            jsr         CLOSE
            ldx         xsave
            rts

; ---- What an rc line can't do

            HEADER      "cd", 0
cd:                                                         ; ( "dir" -- ): forth's current directory (none: $home's)
            LDR         w2, pword
            jsr         sh_word
            bcc         :+
            LDR         r0, s_home
            LDR         r1, pword
            jsr         env_word
            lda         pword
            beq         @done                               ; (No $home: where it is)
:
            LDR         r0, pword
            stx         xsave
            jsr         CHDIR
            ldx         xsave
            bcc         @done
            pha
            LDR         w2, pword
            pla
            jmp         failed
@done:
            rts

            HEADER      "bind", 0
bind:                                                       ; ( "[-a|-b] [-c] new old" -- ): new at old, as rc's
            LDR         p2, s_ubind                         ;   bind (BIND)
            jsr         sh_flags
            LDR         w2, pword
            jsr         sh_word
            bcs         usage
            LDR         w2, pword2
            jsr         sh_word
            bcs         usage
            LDR         r0, pword
            LDR         r1, pword2
            lda         fl
            stx         xsave
            jsr         BIND
            ldx         xsave
            bcc         :+
            pha
            LDR         w2, pword2
            pla
            jmp         failed
:
            rts

; Not as the word's arguments should be: its usage (p2's), and THROW -1 (nothing more said)
usage:
            lda         p2
            sta         w
            lda         p2 + 1
            sta         w + 1
            jsr         type_z
            jsr         cr
            lda         #<-1
            jmp         throw_a

s_ubind:    .byte       "usage: bind [-a|-b] [-c] new old", 0
s_umount:   .byte       "usage: mount [-a|-b] [-c] #x old [spec]", 0
s_uunmount: .byte       "usage: unmount [new] old", 0

            HEADER      "mount", 0
mount:                                                      ; ( "[-a|-b] [-c] #x old [spec]" -- ): device x's server
            LDR         p2, s_umount                        ;   at old, with a spec, as rc's mount (MOUNT)
            jsr         sh_flags
            LDR         w2, pword
            jsr         sh_word
            bcs         @usage
            lda         pword
            cmp         #'#'
            bne         @usage
            lda         pword + 1
            beq         @usage
            LDR         w2, pword2
            jsr         sh_word
            bcs         @usage
            stz         r0
            stz         r0 + 1
            LDR         w2, pword3
            jsr         sh_word
            bcs         :+
            LDR         r0, pword3
:
            LDR         r1, pword2
            lda         fl
            phx
            ldx         pword + 1
            jsr         MOUNT
            plx
            bcc         :+
            pha
            LDR         w2, pword2
            pla
            jmp         failed
:
            rts
@usage:
            jmp         usage

            HEADER      "unmount", 0
unmount:                                                    ; ( "[new] old" -- ): old's mounts and binds, or only
            LDR         p2, s_uunmount                      ;   new's, as rc's unmount (UNMOUNT)
            LDR         w2, pword
            jsr         sh_word
            bcc         :+
            jmp         usage
:
            LDR         w2, pword2
            jsr         sh_word
            bcc         :+
            LDR         r1, pword                           ; (One: old)
            stz         r0
            stz         r0 + 1
            LDR         w2, pword
            bra         @call
:
            LDR         r0, pword
            LDR         r1, pword2
@call:
            stx         xsave
            jsr         UNMOUNT
            ldx         xsave
            bcc         :+
            jmp         failed                              ; (Named by old: w2's)
:
            rts

            HEADER      "newns", 0
newns:                                                      ; ( -- ): the default namespace again (Plan 9's newns:
            jmp         do_newns                            ;   init's and rc's), as forth -l made it

            HEADER      "send", 0
send:                                                       ; ( "n line" -- ): window n's shell gets the line (the
            LDR         p2, s_usend                         ;   rest of this one) as if typed there, and Enter: its
            LDR         w2, pword                           ;   #cN/kbdin (63 keys at most)
            jsr         sh_word
            bcs         @usage
            lda         pword + 1
            bne         @usage
            lda         pword
            cmp         #'0'
            bcc         @usage
            cmp         #'9' + 1
            bcc         @window
@usage:
            jmp         usage
@window:
            ldy         #S_KBDIN_LEN                        ; (#cN/kbdin)
:
            lda         s_kbdin,y
            sta         pword2,y
            dey
            bpl         :-
            lda         pword
            sta         pword2 + 2
            jsr         sh_skip                             ; The line, and a CR
            ldy         #0
:
            jsr         sh_at_end
            bcs         :+
            lda         (p1)
            sta         envbuf,y
            iny
            jsr         sh_next
            cpy         #126
            bcc         :-
:
            lda         #CR
            sta         envbuf,y
            iny
            sty         tmp3
            lda         src_len                             ; (The rest of the line: its)
            sta         to_in
            lda         src_len + 1
            sta         to_in + 1
            LDR         r0, pword2
            lda         #O_WRITE
            stx         xsave
            jsr         OPEN
            ldx         xsave
            bcs         @failed
            sta         tmp3 + 1
            LDR         r0, envbuf
            lda         tmp3
            sta         r1
            stz         r1 + 1
            lda         tmp3 + 1
            stx         xsave
            jsr         WRITE
            php
            pha
            lda         tmp3 + 1
            jsr         CLOSE
            pla
            plp
            ldx         xsave
            bcs         @failed
            rts
@failed:
            pha
            LDR         w2, pword2
            pla
            jmp         failed

s_kbdin:    .byte       "#cN/kbdin", 0
S_KBDIN_LEN = * - s_kbdin - 1
s_usend:    .byte       "usage: send n line", 0

; A system call's failure .A, about the name at w2 (zero-terminated): THROW its ior, the name with its text
failed:
            pha
            lda         w2
            sta         throw_name
            lda         w2 + 1
            sta         throw_name + 1
            ldy         #0
:
            lda         (w2),y
            beq         :+
            iny
            bra         :-
:
            sty         throw_nlen
            lda         #1
            sta         throw_named
            pla
            jmp         throw_os

; fl: the flags' words, rc's (-a after, -b before, -c creates; one word or more, -ac or -a -c).  Not a flag: usage
sh_flags:
            stz         fl
@word:
            jsr         sh_skip
            bcs         @done
            lda         (p1)
            cmp         #'-'
            bne         @done
            jsr         sh_next
@letter:
            jsr         sh_at_end
            bcs         @done
            lda         (p1)
            cmp         #' ' + 1
            bcc         @word
            ldy         #MAFTER
            cmp         #'a'
            beq         :+
            ldy         #MBEFORE
            cmp         #'b'
            beq         :+
            ldy         #MCREATE
            cmp         #'c'
            beq         :+
            jmp         usage
:
            tya
            ora         fl
            sta         fl
            jsr         sh_next
            bra         @letter
@done:
            rts

; ---- The source, rc's way: p1 the next byte (>IN's), sw_left the bytes left

; p1 and sw_left from >IN
sh_at:
            clc
            lda         src_addr
            adc         to_in
            sta         p1
            lda         src_addr + 1
            adc         to_in + 1
            sta         p1 + 1
            sec
            lda         src_len
            sbc         to_in
            sta         sw_left
            rts

; Past the next byte (>IN too)
sh_next:
            inc         p1
            bne         :+
            inc         p1 + 1
:
            dec         sw_left
            inc         to_in
            bne         :+
            inc         to_in + 1
:
            rts

; At the line's end?  OUT: C = 1 yes
sh_at_end:
            lda         sw_left
            beq         :+
            clc
            rts
:
            sec
            rts

; Past blanks: at a word, C = 0; or the line's end, C = 1
sh_skip:
            jsr         sh_at
@blank:
            jsr         sh_at_end
            bcs         @done
            lda         (p1)
            cmp         #' ' + 1
            bcs         @word
            jsr         sh_next
            bra         @blank
@word:
            clc
@done:
            rts

; The next word, rc's way, into w2's buffer (PATH_MAX at most), zero-terminated: '...' quoted ('' a '), $name the
; environment's variable (its first word).  OUT: C = 0, .A its length; or C = 1, none (the line's end)
sh_word:
            jsr         sh_skip
            bcc         :+
            rts
:
            stz         sw_len
            bra         @char
@end:
            lda         #0
            ldy         sw_len
            sta         (w2),y
            tya
            clc
            rts
@char:
            jsr         sh_at_end
            bcs         @end
            lda         (p1)
            cmp         #' ' + 1
            bcc         @end
            cmp         #$27                                ; (')
            beq         @quoted
            cmp         #'$'
            beq         @var
            jsr         @put
            jsr         sh_next
            bra         @char
@quoted:
            jsr         sh_next
@q:
            jsr         sh_at_end
            bcs         @end
            lda         (p1)
            jsr         sh_next
            cmp         #$27
            bne         @qput
            jsr         sh_at_end                           ; ('': a ')
            bcs         @char
            lda         (p1)
            cmp         #$27
            bne         @char
            jsr         sh_next
@qput:
            jsr         @put
            bra         @q
@var:
            jsr         sh_next                             ; $name: its name in envbuf, then its value here
            ldy         #0
@name:
            jsr         sh_at_end
            bcs         @named
            lda         (p1)
            jsr         name_char
            bcs         @named
            sta         envbuf,y
            iny
            jsr         sh_next
            cpy         #31
            bcc         @name
@named:
            lda         #0
            sta         envbuf,y
            LDR         r0, envbuf
            clc                                             ; (Its value at w2's buffer's end)
            lda         w2
            adc         sw_len
            sta         r1
            lda         w2 + 1
            adc         #0
            sta         r1 + 1
            sec
            lda         #PATH_MAX
            sbc         sw_len
            jsr         env_word_n
            clc
            adc         sw_len
            sta         sw_len
            jmp         @char
@put:
            ldy         sw_len
            cpy         #PATH_MAX
            bcs         :+
            sta         (w2),y
            inc         sw_len
:
            rts

; Is .A a name's character (a letter, a digit, _)?  OUT: C = 0 yes; .A kept
name_char:
            cmp         #'_'
            beq         @yes
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         @yes
            pha
            and         #$DF
            cmp         #'A'
            bcc         :+
            cmp         #'Z' + 1
            pla
            rts                                             ; (C = 0 for A-Z)
:
            pla
@no:
            sec
            rts
@yes:
            clc
            rts

; The environment's variable r0's first word into r1's buffer (PATH_MAX at most), zero-terminated ("" if there's
; none).  OUT: .A its length
env_word:
            lda         #PATH_MAX
; ... .A bytes at most
env_word_n:
            sta         r2
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            lda         r1
            sta         w3
            lda         r1 + 1
            sta         w3 + 1
            lda         #$FF
            stx         xsave
            jsr         ENV_GET
            stx         tmp + 1
            ldx         xsave
            bcs         @none
            sta         tmp                                 ; (Its length, at most r2: to its first 0)
            lda         tmp + 1
            bne         :+
            lda         tmp
            cmp         r2
            bcc         :++
:
            lda         r2
            sta         tmp
:
            ldy         #0
:
            cpy         tmp
            beq         @end
            lda         (w3),y
            beq         @end
            iny
            bra         :-
@none:
            ldy         #0
@end:
            lda         #0
            sta         (w3),y
            tya
            rts

; ---- The prompt, statuses, the environment

            HEADER      "prompt", 0
prompt:                                                     ; ( c-addr u -- ): the prompt's format (31 chars at
            LDR         p2, pfmt                            ;   most): %v the card (0:), %d the directory on it, %p
            bra         set_fmt                             ;   the whole path, %t the task, %w the window, %% a %

            HEADER      "prompt2", 0
prompt2:                                                    ; ( c-addr u -- ): the second prompt's, shown while a
            LDR         p2, pfmt2                           ;   definition's being compiled (rc's: a tab, as it starts)
; The format c-addr u (PROMPT_MAX at most) into p2's, counted
set_fmt:
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dhi,x
            beq         :+
            lda         #PROMPT_MAX
            bra         :++
:
            lda         dlo,x
            cmp         #PROMPT_MAX + 1
            bcc         :+
            lda         #PROMPT_MAX
:
            sta         (p2)
            sta         tmp
            inx
            inx
            ldy         #0
:
            cpy         tmp
            beq         :+
            lda         (w),y
            iny
            sta         (p2),y
            bra         :-
:
            rts

            HEADER      "status", 0
status:                                                     ; ( -- n ): the last command's code (an rc line's,
            lda         status_v                            ;   wait's: 0, success)
            ldy         status_v + 1
            PUSHAY
            rts

            HEADER      "exits", 0
exits:                                                      ; ( n -- ): forth ended, its code n (at the console, the
            lda         dlo,x                               ;   line ended first, for the prompt after it, as BYE)
exits_a:
            pha
            lda         interactive
            beq         :+
            lda         lastc
            cmp         #LF
            beq         :+
            jsr         cr
:
            jsr         flush
            stz         r0
            stz         r0 + 1
            pla
            jmp         EXITS

            HEADER      "exit", F_IMMEDIATE
shexit:                                                     ; In a definition, Forth's EXIT; typed at the prompt,
            lda         state                               ;   forth ended, as rc's exit: its code status
            ora         state + 1
            bne         :+
            lda         status_v
            bra         exits_a
:
            lda         #RTS_OP
            jmp         ccomma_a

            HEADER      "wait", 0
wait:                                                       ; ( task -- ): a task this forth started (an rc line with
            lda         dlo,x                               ;   & at its end, spawn's: $apid) waited for: its code
            inx                                             ;   status
            jsr         prog_wait
            jmp         set_status

            HEADER      "getenv", 0
getenv:                                                     ; ( c-addr1 u1 -- c-addr2 u2 ): the environment's variable
            jsr         env_name                            ;   named c-addr1 u1 (u2 0: none; a list's words with
            LDR         r0, pword                           ;   spaces between, rc's)
            LDR         r1, envbuf
            LDR         r2, 127
            stz         r3
            stz         r3 + 1
            lda         #$FF
            stx         xsave
            jsr         ENV_GET
            stx         tmp + 1
            ldx         xsave
            bcc         :+
            lda         #0
            sta         tmp + 1
:
            ldy         tmp + 1                             ; (What fits)
            bne         :+
            cmp         #128
            bcc         :++
:
            lda         #127
:
            tay                                             ; Its 0s: spaces, but at its end
            beq         @push
            dey
            lda         envbuf,y
            bne         :+
            tya
            pha
            bra         @spaces
:
            iny
            tya
            pha
@spaces:
            dey
            bmi         @pushed
            lda         envbuf,y
            bne         @spaces
            lda         #' '
            sta         envbuf,y
            bra         @spaces
@pushed:
            pla
@push:
            pha
            lda         #<envbuf
            ldy         #>envbuf
            PUSHAY
            pla
            ldy         #0
            PUSHAY
            rts

            HEADER      "setenv", 0
setenv:                                                     ; ( c-addr1 u1 c-addr2 u2 -- ): the environment's variable
            lda         dlo + 1,x                           ;   named c-addr1 u1 set to c-addr2 u2 (one word, rc's
            sta         w                                   ;   way: its bytes, then a 0)
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x                               ; (126 bytes at most)
            ldy         dhi,x
            beq         :+
            lda         #126
:
            cmp         #127
            bcc         :+
            lda         #126
:
            sta         tmp3
            inx
            inx
            ldy         #0
:
            cpy         tmp3
            beq         :+
            lda         (w),y
            sta         envbuf,y
            iny
            bra         :-
:
            lda         #0
            sta         envbuf,y
            jsr         env_name
            LDR         r0, pword
            LDR         r1, envbuf
            ldy         tmp3
            iny
            sty         r2
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            lda         #$FF
            stx         xsave
            jsr         ENV_PUT
            ldx         xsave
            bcc         :+
            pha
            LDR         w2, pword
            pla
            jmp         failed
:
            rts

            HEADER      "unsetenv", 0
unsetenv:                                                   ; ( c-addr u -- ): the environment's variable named c-addr
            jsr         env_name                            ;   u removed (there or not)
            LDR         r0, pword
            lda         #$FF
            stx         xsave
            jsr         ENV_DEL
            ldx         xsave
            bcc         :+
            cmp         #E_NOENT
            beq         :+
            pha
            LDR         w2, pword
            pla
            jmp         failed
:
            rts

; ( c-addr u -- ): pword the name, zero-terminated (PATH_MAX at most)
env_name:
            LDR         w2, pword
            jmp         to_z
