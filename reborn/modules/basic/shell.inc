; ****************************************************************************
; shell.inc - basic -l, BASIC as a shell (the eighth bank): HyForth's and hylang's rule (docs/using/basic.md, "The
; shell").  A line is BASIC's if it's a line of the program (a number first), ?, a keyword first, an assignment (a
; name, then = or a subscript's '('), or a call of one of the program's SUBs; any other is an rc command line, run
; whole by rc (rc -c) and waited for ($status its status), or, ending in &, not waited for ($apid its task, in a note
; group of its own).  So pipes, redirections, globbing, quoting and $x are rc's.  % first makes a line rc's at any of
; BASIC's prompts.  What an rc line can't do to BASIC (it runs in a task of its own) the shell does itself, its
; arguments rc's way ('...' quoted, '' a quote; $name the environment's variable, its first word): cd, bind, mount,
; unmount, newns; and exit ends BASIC (its status the last rc line's).  The prompt: the current directory and "> "
; (0:/games> on a card).  basic -l is a login shell: its namespace made (newns), its window's console at /dev, its
; notes its note group's, then /lib/basic/profile.bas's lines run as if typed, then the prompt.

SW_MAX          = NAME_MAX - 1  ; A word's characters at most (sh_word's)
SH_ARGS         = 176           ; rc's arguments (SPAWN's room)
SG_SIZE         = 64            ; (The profile read this much at a time)

.bss
sw_i:       .res        1                                   ; The line's next character (lbuf,sw_i)
sw_len:     .res        1                                   ; A word's length so far; past the first word
sw_n:       .res        1                                   ; (env_first's)
sh_bg:      .res        1                                   ; <> 0: the rc line ends with & (not waited for)
sh_fl:      .res        1                                   ; bind's and mount's flags
sh_src:     .res        1                                   ; The profile: its fd + 1 (0: none)
sh_code:    .res        1                                   ; The last rc line's code (exit's)
sh_err:     .res        1                                   ; (A command's error, newns_do's flags)
sh_flg:     .res        1
sg_buf:     .res        SG_SIZE                             ; (The profile's bytes read, their count, the next)
sg_len:     .res        1
sg_pos:     .res        1
pword:      .res        NAME_MAX                            ; Words read (zero-terminated)
pword2:     .res        NAME_MAX
pword3:     .res        NAME_MAX
envname:    .res        32                                  ; ($name's name)
sh_cwd:     .res        PATH_MAX + 1                        ; (The prompt's directory)
sh_argb:    .res        SH_ARGS                             ; rc's arguments: -c, the line
sh_numb:    .res        8                                   ; (A number's text, for the environment)

.segment "CODE8"

; ---- The prompt

; The prompt in the shell, at the console: on a line of its own, the current directory (on a card, /sd/N: N: and
; the rest of it, or /) and "> "
sh_prompt:
            lda         ocol                                ; (Output since without its LF: one first)
            beq         :+
            lda         #LF
            jsr         out_byte
:
            LDR         r0, sh_cwd
            jsr         GETCWD
            bcs         @end
            jsr         on_card
            ldy         #0
            bcs         @out
            lda         sh_cwd + 4                          ; (N:)
            jsr         out_byte
            lda         #':'
            jsr         out_byte
            ldy         #5
            lda         sh_cwd,y
            bne         @out
            lda         #'/'
            jsr         out_byte
            bra         @end
@out:
            lda         sh_cwd,y
            beq         @end
            jsr         out_byte
            iny
            bra         @out
@end:
            lda         #'>'
            jsr         out_byte
            lda         #' '
            jsr         out_byte
            rts

; Is sh_cwd on a card: /sd/N (N a hex digit), alone or with /... after it?  OUT: C = 0 yes
on_card:
            ldy         #3
:
            lda         sh_cwd,y
            cmp         s_sd,y
            bne         @no
            dey
            bpl         :-
            lda         sh_cwd + 4
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         :+
            cmp         #'a'
            bcc         @no
            cmp         #'f' + 1
            bcs         @no
:
            lda         sh_cwd + 5
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

; ---- A line's rule

; The line read (lbuf): % first, rc's (at any prompt); in the shell, by the rule: BASIC's, or rc's, or the shell's own
; (cd, bind, mount, unmount, newns, exit).  OUT: C = 1 it's been run (not BASIC's); C = 0 BASIC's
sh_line:
            ldx         #0
            jsr         sw_skipx
            lda         lbuf,x
            bne         @lb903
            jmp         @basic
@lb903:
            cmp         #'%'
            bne         :+
            inx
            stx         sw_i
            jmp         rc_line
:
            lda         login
            bne         @lb902
            jmp         @basic
@lb902:
            lda         lbuf,x
            cmp         #'?'
            bne         @lb901
            jmp         @basic
@lb901:
            cmp         #'0'
            bcc         :+
            cmp         #'9' + 1
            bcs         @lb900                              ; (A line's number)
            jmp         @basic
@lb900:
:
            stx         sw_i                                ; Its first word: a name?
            ldy         #0
@name:
            lda         lbuf,x
            jsr         name_charu
            bcs         @named
            cpy         #0                                  ; (A letter first)
            bne         :+
            cmp         #'A'
            bcc         @named
:
            sta         tname,y
            sta         pword,y
            inx
            iny
            cpy         #SW_MAX
            bcc         @name
@named:
            cpy         #0
            beq         @rc                                 ; (Not a name: rc's)
            lda         #0
            sta         pword,y
            sty         tnl
            stz         tnl + 1
            stz         tsuf
            lda         lbuf,x                              ; (A string's name: $ after it)
            cmp         #'$'
            bne         :+
            sta         tsuf
            inx
:
            stx         sw_len                              ; (Past the name)
            jsr         builtin                             ; The shell's own?
            bcs         @done
            FARN        2, kw_find                          ; A keyword?
            bcc         @basic
            ldx         sw_len                              ; An assignment (= or a subscript after the name)?
            jsr         sw_skipx
            lda         lbuf,x
            cmp         #'='
            beq         @basic
            cmp         #'('
            beq         @basic
            lda         tsuf                                ; A SUB of the program's?
            bne         @rc
            FARN        2, proc_find
            bcc         @basic
@rc:
            jmp         rc_line
@basic:
            clc
            rts
@done:
            sec
            rts

; .X past blanks in lbuf
sw_skipx:
            lda         lbuf,x
            beq         :+
            cmp         #' ' + 1
            bcs         :+
            inx
            bra         sw_skipx
:
            rts

; Is .A a name's character (a letter, a digit, _ or .)?  OUT: C = 0, .A it in upper case; C = 1 no
name_charu:
            cmp         #'0'
            bcc         @dot
            cmp         #'9' + 1
            bcc         @yes
            cmp         #'_'
            beq         @yes
            and         #$DF
            cmp         #'A'
            bcc         @no
            cmp         #'Z' + 1
            rts                                             ; (C = 0 for A-Z)
@dot:
            cmp         #'.'
            beq         @yes
@no:
            sec
            rts
@yes:
            clc
            rts

; ---- rc's lines

; The line from lbuf,sw_i run by rc (rc -c), waited for: $status its status (its message, or its code); ending in &
; (not &&) not waited for ($apid its task, in a note group of its own).  Ctrl-C ends it (rc's), and the shell goes
; on, on a new line.  OUT: C = 1
rc_line:
            ldx         sw_i                                ; Its end: blanks left off, and an &
            jsr         sw_skipx
            stx         sw_i
            ldy         sw_i
:
            lda         lbuf,y
            beq         :+
            iny
            bra         :-
:
            stz         sh_bg
@trail:
            cpy         sw_i
            beq         @args
            dey
            lda         lbuf,y
            cmp         #' ' + 1
            bcc         @trail
            cmp         #'&'
            bne         @end
            cpy         sw_i
            beq         :+
            lda         lbuf - 1,y
            cmp         #'&'
            beq         @end
:
            inc         sh_bg
            bra         @args
@end:
            iny                                             ; (.Y: past its last character)
@args:
            sty         sw_len
            lda         #'-'                                ; sh_argb: -c, 0, the line, 0, 0
            sta         sh_argb
            lda         #'c'
            sta         sh_argb + 1
            stz         sh_argb + 2
            ldx         sw_i
            ldy         #3
:
            cpx         sw_len
            beq         :+
            lda         lbuf,x
            sta         sh_argb,y
            inx
            iny
            cpy         #SH_ARGS - 2
            bcc         :-
            lda         #E_NAMETOOLONG                      ; (Too long for rc's arguments)
            jmp         sh_fail0
:
            lda         #0
            sta         sh_argb,y
            sta         sh_argb + 1,y
            jsr         out_flush
            FARN        6, con_cooked
            LDW         sh_args, sh_argb
            lda         #0
            ldx         sh_bg
            beq         :+
            lda         #SPAWN_NEWGROUP
:
            FARN        7, rc_spawn
            bcc         :+
            jmp         sh_fail0
:
            ldx         sh_bg
            beq         @wait
            jsr         sh_numtx                            ; ($apid: its task; status 0)
            LDR         r0, s_apid
            jsr         env_put_num
            stz         sh_code
            stz         esbuf
            jmp         set_status
@wait:
            stz         esbuf
            FARN        7, rc_wait
            lda         sh_status
            sta         sh_code
            bit         intr                                ; (Ctrl-C ended it: the shell goes on, on a new line)
            bpl         :+
            stz         intr
            lda         #LF
            jsr         out_byte
            stz         ocol
:
            jmp         set_status

s_apid:     .byte       "apid", 0
s_status:   .byte       "status", 0
s_home:     .byte       "home", 0
s_window:   .byte       "window", 0

; $status = esbuf (the exit's message, rc's), or if it's empty sh_code's number.  OUT: C = 1
set_status:
            LDR         r0, s_status
            LDR         r1, esbuf
            lda         esbuf
            bne         :+
            lda         sh_code
            jsr         sh_numtx
            LDR         r1, sh_numb
:
            jsr         env_put
            sec
            rts

; The environment's variable r0 = sh_numb
env_put_num:
            LDR         r1, sh_numb
; ... = r1 (zero-terminated: its text and its 0, one word, rc's way).  A failure: no matter
env_put:
            jsr         ram_path
            ldy         #0
:
            lda         (r1),y
            beq         :+
            iny
            bra         :-
:
            iny
            sty         r2
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            lda         #$FF
            jmp         ENV_PUT

; r0's name (this bank's) copied to envname, r0 it (calls read their names in the RAM)
ram_path:
            ldy         #0
:
            lda         (r0),y
            sta         envname,y
            beq         :+
            iny
            cpy         #31
            bcc         :-
            lda         #0
            sta         envname,y
:
            LDR         r0, envname
            rts

; sh_numb: .A in decimal, zero-terminated
sh_numtx:
            ldy         #0
            ldx         #100
            jsr         @digit
            ldx         #10
            jsr         @digit
            ora         #'0'
            sta         sh_numb,y
            lda         #0
            sta         sh_numb + 1,y
            rts
@digit:                                                     ; (.A's digit for .X, kept if it isn't a leading 0)
            stx         sw_len
            ldx         #0
:
            cmp         sw_len
            bcc         :+
            sbc         sw_len
            inx
            bra         :-
:
            pha
            txa
            bne         :+
            cpy         #0
            beq         :++
:
            ora         #'0'
            sta         sh_numb,y
            iny
:
            pla
            rts

; A shell command that failed (.A its error): "word: its error" (t4's word, the system's text) on a line, $status
; its text; sh_fail0: the line itself named.  OUT: C = 1
sh_fail0:
            stz         pword
            LDW         t4, pword
sh_fail:
            sta         sh_err
            ldy         #0
:
            lda         (t4),y
            beq         :+
            jsr         out_byte
            iny
            bra         :-
:
            cpy         #0
            beq         :+
            lda         #':'
            jsr         out_byte
            lda         #' '
            jsr         out_byte
:
            LDR         r0, esbuf
            lda         sh_err
            jsr         ERRSTR
            ldy         #0
:
            lda         esbuf,y
            beq         :+
            jsr         out_byte
            iny
            bra         :-
:
            lda         #LF
            jsr         out_byte
            stz         ocol
            lda         #1
            sta         sh_code
            jmp         set_status

; ---- The shell's own commands

; Is pword one of the shell's own commands (cd, bind, mount, unmount, newns, exit, any case: upper in pword)?  Then
; run it, the line after the name its arguments (sw_len), and C = 1; else C = 0
builtin:
            ldx         #0
@cmd:
            lda         sh_cmds,x
            beq         @no
            ldy         #0
@char:
            lda         sh_cmds,x
            cmp         pword,y
            bne         @skip
            inx
            iny
            lda         sh_cmds,x
            bpl         @char                               ; (Its routine next: an address's high byte, bit 7 set)
            lda         pword,y
            bne         @past
            lda         sw_len
            sta         sw_i
            lda         sh_cmds + 1,x                       ; (Its routine: high, low)
            pha
            lda         sh_cmds + 2,x
            pha
            rts                                             ; (rts to it: its address less 1)
@skip:
            inx
            lda         sh_cmds,x
            bpl         @skip
@past:
            inx                                             ; (Past the routine's two bytes, and its marker)
            inx
            inx
            bra         @cmd
@no:
            clc
            rts

.macro shcmd name, routine
            .byte       name, $80
            .byte       >(routine - 1), <(routine - 1)
.endmacro
sh_cmds:
            shcmd       "CD", sh_cd
            shcmd       "BIND", sh_bind
            shcmd       "MOUNT", sh_mount
            shcmd       "UNMOUNT", sh_unmount
            shcmd       "NEWNS", sh_newns
            shcmd       "EXIT", sh_exit
            .byte       0

; cd [dir]: BASIC's current directory (none: $home's)
sh_cd:
            LDW         t4, pword
            jsr         sh_word
            bcc         :+
            LDR         r0, s_home
            jsr         env_pword
            lda         pword
            beq         @done                               ; (No $home: where it is)
:
            LDR         r0, pword
            jsr         CHDIR
            bcc         @done
            jmp         sh_fail                             ; (t4: pword, sh_word's)
@done:
            jmp         sh_ok

; bind [-a|-b] [-c] new old: new at old (BIND), as rc's
sh_bind:
            jsr         sh_flags
            bcs         @usage
            LDW         t4, pword
            jsr         sh_word
            bcs         @usage
            LDW         t4, pword2
            jsr         sh_word
            bcs         @usage
            LDR         r0, pword
            LDR         r1, pword2
            lda         sh_fl
            jsr         BIND
            bcs         :+
            jmp         sh_ok
:
            jmp         sh_fail                             ; (t4: pword2, old)
@usage:
            LDW         t0, s_ubind
            jmp         sh_usage

; mount [-a|-b] [-c] #x old [spec]: device x's server at old (MOUNT), as rc's
sh_mount:
            jsr         sh_flags
            bcs         @usage
            LDW         t4, pword
            jsr         sh_word
            bcs         @usage
            lda         pword
            cmp         #'#'
            bne         @usage
            lda         pword + 1
            beq         @usage
            LDW         t4, pword2
            jsr         sh_word
            bcs         @usage
            stz         r0
            stz         r0 + 1
            LDW         t4, pword3
            jsr         sh_word
            bcs         :+
            LDR         r0, pword3
:
            LDR         r1, pword2
            lda         sh_fl
            ldx         pword + 1
            jsr         MOUNT
            bcs         :+
            jmp         sh_ok
:
            pha
            LDW         t4, pword2
            pla
            jmp         sh_fail
@usage:
            LDW         t0, s_umount
            jmp         sh_usage

; unmount [new] old: old's mounts and binds, or new's only (UNMOUNT), as rc's
sh_unmount:
            LDW         t4, pword
            jsr         sh_word
            bcs         @usage
            LDW         t4, pword2
            jsr         sh_word
            bcc         :+
            LDR         r1, pword                           ; (One: old)
            stz         r0
            stz         r0 + 1
            bra         @call
:
            LDR         r0, pword
            LDR         r1, pword2
@call:
            jsr         UNMOUNT
            bcs         :+
            jmp         sh_ok
:
            pha
            LDW         t4, pword
            pla
            jmp         sh_fail
@usage:
            LDW         t0, s_uunmount
            jmp         sh_usage

; The command's done: status 0.  OUT: C = 1
sh_ok:
            stz         sh_code
            stz         esbuf
            jmp         set_status

; The usage at (t0) said, status 1.  OUT: C = 1
sh_usage:
            ldy         #0
:
            lda         (t0),y
            beq         :+
            jsr         out_byte
            iny
            bra         :-
:
            lda         #LF
            jsr         out_byte
            stz         ocol
            lda         #1
            sta         sh_code
            stz         esbuf
            jmp         set_status

s_ubind:    .byte       "usage: bind [-a|-b] [-c] new old", 0
s_umount:   .byte       "usage: mount [-a|-b] [-c] #x old [spec]", 0
s_uunmount: .byte       "usage: unmount [new] old", 0

; newns: the default namespace again (nslib's: Plan 9's newns, init's and rc's)
sh_newns:
            jsr         newns_do
            bcs         :+
            jmp         sh_ok
:
            jmp         sh_fail0

; exit: BASIC's end, its status the last rc line's
sh_exit:
            jsr         out_flush
            FARN        6, con_cooked
            LDR         r0, 0
            lda         sh_code
            jmp         EXITS

; The namespace made, as newns makes it (nslib's ns_default), its buffers at $8000 in a bank taken for the while.
; OUT: C = 0; or C = 1, .A the error (E_NOMEM: no bank to be had)
newns_do:
            lda         #1
            jsr         BANKS_ALLOC
            bcc         :+
            lda         #E_NOMEM
            rts
:
            pha
            sta         RAM_BANK
            FARN        1, ns_default
            sta         sh_err
            php
            pla
            sta         sh_flg
            jsr         code_sel
            pla                                             ; (The bank given back)
            ldx         #1
            jsr         BANKS_FREE
            lda         sh_flg                              ; (ns_default's C, its error)
            pha
            lda         sh_err
            plp
            rts

; ---- Words, rc's way

; sh_fl: the flags' words (-a after, -b before, -c creates: one word or more, -ac or -a -c).  OUT: C = 1, one that
; isn't a flag
sh_flags:
            stz         sh_fl
@word:
            ldx         sw_i
            jsr         sw_skipx
            stx         sw_i
            lda         lbuf,x
            cmp         #'-'
            bne         @done
            inx
@letter:
            lda         lbuf,x
            cmp         #' ' + 1
            bcc         @next
            ldy         #MAFTER
            cmp         #'a'
            beq         :+
            ldy         #MBEFORE
            cmp         #'b'
            beq         :+
            ldy         #MCREATE
            cmp         #'c'
            beq         :+
            sec
            rts
:
            tya
            ora         sh_fl
            sta         sh_fl
            inx
            bra         @letter
@next:
            stx         sw_i
            bra         @word
@done:
            clc
            rts

; The next word from lbuf,sw_i into (t4)'s buffer (SW_MAX characters), zero-terminated, rc's way: '...' quoted ('' a
; quote), $name the environment's variable (its first word).  OUT: C = 0; or C = 1, none (the line's end)
sh_word:
            ldx         sw_i
            jsr         sw_skipx
            lda         lbuf,x
            bne         :+
            stx         sw_i
            sec
            rts
:
            stz         sw_len
@char:
            lda         lbuf,x
            cmp         #' ' + 1
            bcc         @end
            cmp         #$27                                ; (')
            beq         @quoted
            cmp         #'$'
            beq         @var
            jsr         @put
            inx
            bra         @char
@end:
            stx         sw_i
            ldy         sw_len
            lda         #0
            sta         (t4),y
            clc
            rts
@quoted:
            inx
@q:
            lda         lbuf,x
            beq         @end
            inx
            cmp         #$27
            bne         @qput
            lda         lbuf,x                              ; ('': a ')
            cmp         #$27
            bne         @char
            inx
@qput:
            jsr         @put
            bra         @q
@var:
            inx                                             ; $name: its name in envname, then its value here
            ldy         #0
@name:
            lda         lbuf,x
            cmp         #'_'
            beq         :+
            jsr         name_charu
            bcs         @named
            lda         lbuf,x                              ; (As it was: the name's case kept)
:
            sta         envname,y
            iny
            inx
            cpy         #31
            bcc         @name
@named:
            lda         #0
            sta         envname,y
            phx
            LDR         r0, envname
            clc                                             ; (Its value after the word so far)
            lda         t4
            adc         sw_len
            sta         r1
            lda         t4 + 1
            adc         #0
            sta         r1 + 1
            sec
            lda         #SW_MAX
            sbc         sw_len
            jsr         env_first
            clc
            adc         sw_len
            sta         sw_len
            plx
            jmp         @char
@put:
            ldy         sw_len
            cpy         #SW_MAX
            bcs         :+
            sta         (t4),y
            inc         sw_len
:
            rts

; pword = the environment's variable r0's (this bank's name) first word
env_pword:
            jsr         ram_path
            LDR         r1, pword
            lda         #SW_MAX
; The environment's variable r0's first word into r1's buffer, .A bytes at most, zero-terminated ("" if it isn't
; set).  OUT: .A its length
env_first:
            sta         r2
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            lda         r1
            pha
            lda         r1 + 1
            pha
            lda         #$FF
            jsr         ENV_GET
            ply                                             ; (r1 back, in t0: the buffer)
            sty         t0 + 1
            ply
            sty         t0
            bcs         @none
            cpx         #0                                  ; (Its length, at most r2's)
            bne         :+
            cmp         r2
            bcc         :++
:
            lda         r2
:
            sta         sw_n
            ldy         #0
:
            cpy         sw_n
            beq         @end
            lda         (t0),y
            beq         @end
            iny
            bra         :-
@none:
            ldy         #0
@end:
            lda         #0
            sta         (t0),y
            tya
            rts

; ---- basic -l: the login

; The login shell's start (main's, for -l): its namespace (newns), its window's console at /dev (not window 0's:
; #c's taken off it, #c$window put after it, as rc's /lib/profile does), its notes its note group's (/dev/consctl's
; group); then /lib/basic/profile.bas opened, its lines run as typed ones (sh_getline), then the prompt
sh_login:
            jsr         newns_do
            LDR         r0, s_window                        ; Its window: not 0?
            jsr         env_pword
            lda         pword
            beq         @group
            cmp         #'0'
            bne         :+
            lda         pword + 1
            beq         @group
:
            LDR         r0, s_conc                          ; unmount '#c' /dev
            jsr         ram_path
            ldx         #4
:
            lda         s_dev,x
            sta         pword3,x
            dex
            bpl         :-
            LDR         r1, pword3
            jsr         UNMOUNT
            lda         #'#'                                ; bind -a '#c'$window /dev
            sta         pword2
            lda         #'c'
            sta         pword2 + 1
            ldy         #0
:
            lda         pword,y
            sta         pword2 + 2,y
            beq         :+
            iny
            cpy         #SW_MAX - 2
            bcc         :-
:
            LDR         r0, pword2
            LDR         r1, pword3
            lda         #MAFTER
            jsr         BIND
@group:
            LDR         r0, s_shctl                       ; echo -n group >/dev/consctl
            jsr         ram_path
            lda         #O_WRITE
            jsr         OPEN
            bcs         @profile
            pha
            ldx         #4
:
            lda         s_group,x
            sta         pword3,x
            dex
            bpl         :-
            LDR         r0, pword3
            LDR         r1, 5
            pla
            pha
            jsr         WRITE
            pla
            jsr         CLOSE
@profile:
            ldy         #0                                  ; The profile, if there's one, as typed
:
            lda         s_profile,y
            sta         sh_cwd,y
            beq         :+
            iny
            bra         :-
:
            LDR         r0, sh_cwd
            lda         #O_READ
            jsr         OPEN
            bcs         :+
            inc         a
            sta         sh_src
            stz         sg_len
            stz         sg_pos
:
            rts

s_conc:     .byte       "#c", 0
s_dev:      .byte       "/dev", 0
s_shctl:  .byte       "/dev/consctl", 0
s_group:    .byte       "group"
s_profile:  .byte       "/lib/basic/profile.bas", 0

; The profile's next line into lbuf (zero-terminated; its CR dropped).  OUT: C = 0 a line; C = 1 its end (closed:
; the console's lines after)
sh_getline:
            ldx         #0
@byte:
            phx
            jsr         sg_getc
            plx
            bcs         @end
            cmp         #CR
            beq         @byte
            cmp         #LF
            beq         @done
            cpx         #255
            bcs         @byte
            sta         lbuf,x
            inx
            bra         @byte
@end:
            cpx         #0
            bne         @done
            lda         sh_src
            dec         a
            jsr         CLOSE
            stz         sh_src
            sec
            rts
@done:
            stz         lbuf,x
            clc
            rts

; The profile's next byte.  OUT: C = 1 at its end
sg_getc:
            ldx         sg_pos
            cpx         sg_len
            bcc         @have
            LDR         r0, sg_buf
            LDR         r1, SG_SIZE
            lda         sh_src
            dec         a
            jsr         READ
            bcs         @end
            sta         sg_len
            stz         sg_pos
            cmp         #0
            beq         @end
            ldx         #0
@have:
            lda         sg_buf,x
            inx
            stx         sg_pos
            clc
            rts
@end:
            sec
            rts
