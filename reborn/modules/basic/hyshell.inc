; ****************************************************************************
; hyshell.inc - BASIC as a shell (included by basic.s): HyForth's and hylang's rule (docs/hyforth.md, "The shell";
; docs/basic.md).  At the prompt (basic -l's) a line is BASIC's if it's a line of the program (a number first), ?, a
; statement's keyword as its first word, or an assignment (a name, then = or a subscript's '('); any other is an rc
; command line, run whole by rc (rc -c) and waited for, its code $status (and exit's); one ending in & isn't waited
; for (its task: $apid, in a note group of its own).  So pipes, redirections, globbing, quoting and $x are rc's.  A
; name in both is BASIC's (sleep, wait, if, for: % before them for rc's).  What an rc line can't do to BASIC (it runs
; in a task of its own: BASIC's current directory and namespace stay as they were) the shell does itself, its
; arguments rc's way ('...' quoted, '' a quote; $name the environment's variable, its first word): cd, bind, mount,
; unmount, newns; and exit ends BASIC (its code $status's).  % before a line makes it rc's whatever it is, at any
; BASIC's prompt.  The prompt is HyForth's and hylang's: the directory and "> " (0:/games> on a card).  basic -l is a
; login shell: its namespace made (newns: nslib, its buffers in a bank of the task's meanwhile), its window's console
; at /dev, its notes its group's, then /lib/basic/profile.bas run as typed lines, then the prompt.

SW_MAX      = NAME_MAX - 1                                  ; A word's characters at most (sh_word)

.bss
shell:      .res        1                                   ; <> 0: the shell's rule (basic -l)
ld_exec:    .res        1                                   ; <> 0: LOAD's source's lines run as typed (the profile)
sh_status:  .res        1                                   ; The last rc line's code (exit's)
sh_bg:      .res        1                                   ; <> 0: the rc line ends with & (not waited for)
sw_i:       .res        1                                   ; The line's next character (INPUTBUFFER,sw_i)
sw_len:     .res        1                                   ; sh_word's word so far
sh_fl:      .res        1                                   ; bind's and mount's flags
pword:      .res        NAME_MAX                            ; Words parsed (zero-terminated)
pword2:     .res        NAME_MAX
pword3:     .res        NAME_MAX
envname:    .res        32                                  ; $name's name, ENV$'s
argbuf:     .res        ARGS_MAX                            ; rc's arguments (-c, the line)
statbuf:    .res        32                                  ; An exit's message (WAIT's)
numbuf:     .res        8                                   ; A number's text, for the environment
.code

; ---- The prompt, and a line's rule

; The prompt before a line typed at the console in the shell (L2351's): on a line of its own, the current directory
; (on a card, /sd/N: N: and the rest of it, or /) and "> "
SHELL_PROMPT:
            lda         shell
            beq         @done
            lda         interactive
            beq         @done
            lda         cur_src
            bne         @done
            lda         POSX                                ; (Output since without its LF: one first)
            beq         :+
            jsr         CRDO
:
            LDR         r0, pword
            jsr         GETCWD
            bcs         @end
            jsr         on_card
            ldy         #0
            bcs         @out
            lda         pword + 4                           ; (N:)
            jsr         OUTDO
            lda         #':'
            jsr         OUTDO
            ldy         #5
            lda         pword,y
            bne         @out
            lda         #'/'
            jsr         OUTDO
            bra         @end
@out:
            lda         pword,y
            beq         @end
            jsr         OUTDO
            iny
            bra         @out
@end:
            lda         #'>'
            jsr         OUTDO
            lda         #' '
            jsr         OUTDO
            jsr         FLUSH
            stz         POSX                                ; (The line typed after it ends with the console's new line)
@done:
            rts

; Is pword (a current directory) on a card: /sd/N (N a hex digit), alone or with /... after it?  OUT: C = 0 yes
on_card:
            ldy         #3
:
            lda         pword,y
            cmp         s_sd,y
            bne         @no
            dey
            bpl         :-
            lda         pword + 4
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         :+
            cmp         #'a'
            bcc         @no
            cmp         #'f' + 1
            bcs         @no
:
            lda         pword + 5
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

; The line INLIN read (INPUTBUFFER, as typed): % first, rc's (any BASIC's); in the shell (the console's or the
; profile's line), by the rule: BASIC's, or rc's, or the shell's own (cd, bind, mount, unmount, newns, exit).  OUT:
; C = 1, it's been run (not BASIC's); C = 0, BASIC's
SHELL_LINE:
            lda         cur_src                             ; (LOAD's lines: not the shell's; the profile's yes)
            beq         :+
            lda         ld_exec
            beq         @basic
:
            ldx         #0
            jsr         sw_skipx
            lda         INPUTBUFFER,x
            beq         @basic
            cmp         #'%'
            bne         :+
            inx
            stx         sw_i
            jmp         RC_LINE
:
            lda         shell
            beq         @basic
            lda         INPUTBUFFER,x
            cmp         #'?'
            beq         @basic
            cmp         #'0'
            bcc         :+
            cmp         #'9' + 1
            bcc         @basic                              ; (A line's number)
:
            stx         sw_i                                ; Its first word: a name?
            ldy         #0
@name:
            lda         INPUTBUFFER,x
            jsr         name_charu
            bcs         @named
            sta         pword,y
            inx
            iny
            cpy         #SW_MAX
            bcc         @name
@named:
            cpy         #0
            beq         @rc                                 ; (Not a name: rc's)
            lda         INPUTBUFFER,x                       ; (A string's or an integer's name: $ or % after it)
            cmp         #'$'
            beq         :+
            cmp         #'%'
            bne         :++
:
            sta         pword,y
            inx
            iny
:
            lda         #0
            sta         pword,y
            stx         sw_len                              ; (Past the name)
            jsr         BUILTIN                             ; The shell's own?
            bcs         @done
            jsr         IS_STATEMENT                        ; A statement's keyword?
            bcc         @basic
            ldx         sw_len                              ; An assignment (= or a subscript after the name)?
            jsr         sw_skipx
            lda         INPUTBUFFER,x
            cmp         #'='
            beq         @basic
            cmp         #'('
            beq         @basic
@rc:
            jmp         RC_LINE
@basic:
            clc
            rts
@done:
            sec
            rts

; .X past blanks in INPUTBUFFER
sw_skipx:
            lda         INPUTBUFFER,x
            beq         :+
            cmp         #' ' + 1
            bcs         :+
            inx
            bra         sw_skipx
:
            rts

; Is .A a name's character (a letter, a digit)?  OUT: C = 0, .A it in upper case; C = 1 no
name_charu:
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         @yes
            and         #$DF
            cmp         #'A'
            bcc         @no
            cmp         #'Z' + 1
            rts                                             ; (C = 0 for A-Z)
@yes:
            clc
            rts
@no:
            sec
            rts

; Is pword (in upper case) a statement's keyword, or an alias of one?  OUT: C = 0 yes
IS_STATEMENT:
            jsr         KW_FIRST
            lda         #$80                                ; (The token)
@kw:
            pha
            ldy         #0
@char:
            lda         (KWPTR),y
            beq         @end                                ; (The table's end)
            and         #$7F
            cmp         pword,y
            bne         @next
            lda         (KWPTR),y
            iny
            asl         a
            bcc         @char
            lda         pword,y                             ; (All of it)
            bne         @next
            pla                                             ; A keyword: a statement's?
            cmp         #FIRST_ALIAS
            bcc         :+
            sbc         #FIRST_ALIAS
            tay
            lda         ALIAS_MAP,y
:
            cmp         #$80 + NUM_TOKENS
            rts                                             ; (C = 0: a statement's)
@next:
            jsr         KW_NEXT
            pla
            inc         a
            bne         @kw
            pha
@end:
            pla
            sec
            rts

; ---- rc's lines

; The line from INPUTBUFFER,sw_i run by rc (rc -c), waited for: its code sh_status, and $status (its message, or
; its code); with & at its end not waited for ($apid its task, in a note group of its own).  Ctrl-C ends it (rc's),
; and the shell goes on, on a new line.  OUT: C = 1
RC_LINE:
            ldx         sw_i                                ; Its end: blanks left off, and an & (not &&)
            jsr         sw_skipx
            stx         sw_i
            ldy         sw_i
:
            lda         INPUTBUFFER,y
            beq         :+
            iny
            bra         :-
:
            stz         sh_bg
@trail:
            cpy         sw_i
            beq         @args
            dey
            lda         INPUTBUFFER,y
            cmp         #' ' + 1
            bcc         @trail
            cmp         #'&'
            bne         @end
            cpy         sw_i
            beq         :+
            lda         INPUTBUFFER-1,y
            cmp         #'&'
            beq         @end
:
            inc         sh_bg
            bra         @args
@end:
            iny                                             ; (.Y: past its last character)
@args:
            sty         sw_len
            lda         #'-'                                ; argbuf: -c, 0, the line, 0, 0
            sta         argbuf
            lda         #'c'
            sta         argbuf + 1
            stz         argbuf + 2
            ldx         sw_i
            ldy         #3
:
            cpx         sw_len
            beq         :+
            lda         INPUTBUFFER,x
            sta         argbuf,y
            inx
            iny
            cpy         #ARGS_MAX - 2
            bcc         :-
            lda         #E_NAMETOOLONG                      ; (Too long for rc's arguments)
            jmp         SH_FAIL0
:
            lda         #0
            sta         argbuf,y
            sta         argbuf + 1,y
            jsr         FLUSH
            lda         sh_bg                               ; rc: /bin/rc, else the ROM's (#m/rc)
            beq         :+
            lda         #SPAWN_NEWGROUP
:
            pha
            LDR         r0, s_binrc
            pla
            pha
            jsr         RC_SPAWN
            bcc         @started
            cmp         #E_NOENT
            bne         @fail
            LDR         r0, s_mrc
            pla
            pha
            jsr         RC_SPAWN
            bcc         @started
@fail:
            ply
            jmp         SH_FAIL0
@started:
            ply
            ldy         sh_bg
            beq         @wait
            jsr         num_text                            ; ($apid: its task; status 0)
            LDR         r0, s_apid
            jsr         env_put_num
            stz         sh_status
            stz         statbuf
            jmp         SET_STATUS
@wait:
            sta         sw_len                              ; (The task)
:
            LDR         r0, statbuf
            lda         sw_len
            jsr         WAIT
            bcc         :+
            cmp         #E_INTR                             ; (A note: waited for again; another error: as if 0)
            beq         :-
            ldx         #0
            stz         statbuf
:
            stx         sh_status
            bit         intr                                ; (Ctrl-C ended it: the shell goes on, on a new line)
            bpl         :+
            stz         intr
            jsr         CRDO
:
            jmp         SET_STATUS

; SPAWN r0's program with argbuf, .A its flags.  OUT: as SPAWN's
RC_SPAWN:
            pha
            LDR         r1, argbuf
            pla
            jmp         SPAWN

s_binrc:    .byte       "/bin/rc", 0
s_mrc:      .byte       "#m/rc", 0
s_apid:     .byte       "apid", 0
s_status:   .byte       "status", 0
s_home:     .byte       "home", 0
s_window:   .byte       "window", 0

; $status = statbuf (the exit's message, rc's), or if it's empty sh_status's number.  OUT: C = 1
SET_STATUS:
            LDR         r0, s_status
            LDR         r1, statbuf
            lda         statbuf
            bne         :+
            lda         sh_status
            jsr         num_text
            LDR         r1, numbuf
:
            jsr         env_put
            sec
            rts

; The environment's variable r0 = numbuf
env_put_num:
            LDR         r1, numbuf
; ... = r1 (zero-terminated: its text and its 0, one word, rc's way).  A failure: no matter
env_put:
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

; numbuf: .A in decimal, zero-terminated
num_text:
            ldy         #0
            ldx         #100
            jsr         @digit
            ldx         #10
            jsr         @digit
            ora         #'0'
            sta         numbuf,y
            lda         #0
            sta         numbuf + 1,y
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
            sta         numbuf,y
            iny
:
            pla
            rts

; A shell command that failed: "word: its error" (w2's word, the system's text), on a line, and $status its text;
; SH_FAIL0: the line itself named.  OUT: C = 1
SH_FAIL0:
            ldx         #<pword
            ldy         #>pword
            stz         pword
            bra         :+
SH_FAIL:
            ldx         IOPTR
            ldy         IOPTR+1
:
            sta         os_err
            stx         IOPTR
            sty         IOPTR+1
            ldy         #0
:
            lda         (IOPTR),y
            beq         :+
            jsr         OUTDO
            iny
            bra         :-
:
            cpy         #0
            beq         :+
            lda         #':'
            jsr         OUTDO
            lda         #' '
            jsr         OUTDO
:
            LDR         r0, statbuf
            lda         os_err
            jsr         ERRSTR
            ldy         #0
:
            lda         statbuf,y
            beq         :+
            jsr         OUTDO
            iny
            bra         :-
:
            jsr         CRDO
            lda         #1
            sta         sh_status
            jmp         SET_STATUS

; ---- The shell's own commands

; Is pword one of the shell's own commands (cd, bind, mount, unmount, newns, exit, any case)?  Then run it, the line
; after the name its arguments (sw_len), and C = 1; else C = 0
BUILTIN:
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
            lda         sh_cmds+1,x                         ; (Its routine: high, low)
            pha
            lda         sh_cmds+2,x
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
            shcmd       "CD", SH_CD
            shcmd       "BIND", SH_BIND
            shcmd       "MOUNT", SH_MOUNT
            shcmd       "UNMOUNT", SH_UNMOUNT
            shcmd       "NEWNS", SH_NEWNS
            shcmd       "EXIT", SH_EXIT
            .byte       0

; cd [dir]: BASIC's current directory (none: $home's)
SH_CD:
            LDR         IOPTR, pword
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
            jmp         SH_FAIL                             ; (IOPTR: pword, sh_word's)
@done:
            stz         sh_status
            sec
            rts

; bind [-a|-b] [-c] new old: new at old (BIND), as rc's
SH_BIND:
            jsr         sh_flags
            bcs         @usage
            LDR         IOPTR, pword
            jsr         sh_word
            bcs         @usage
            LDR         IOPTR, pword2
            jsr         sh_word
            bcs         @usage
            LDR         r0, pword
            LDR         r1, pword2
            lda         sh_fl
            jsr         BIND
            jcc         SH_OK
            jmp         SH_FAIL                             ; (IOPTR: pword2, old)
@usage:
            lda         #<s_ubind
            ldy         #>s_ubind
            jmp         SH_USAGE

; mount [-a|-b] [-c] #x old [spec]: device x's server at old (MOUNT), as rc's
SH_MOUNT:
            jsr         sh_flags
            bcs         @usage
            LDR         IOPTR, pword
            jsr         sh_word
            bcs         @usage
            lda         pword
            cmp         #'#'
            bne         @usage
            lda         pword + 1
            beq         @usage
            LDR         IOPTR, pword2
            jsr         sh_word
            bcs         @usage
            stz         r0
            stz         r0 + 1
            LDR         IOPTR, pword3
            jsr         sh_word
            bcs         :+
            LDR         r0, pword3
:
            LDR         r1, pword2
            lda         sh_fl
            ldx         pword + 1
            jsr         MOUNT
            jcc         SH_OK
            pha
            LDR         IOPTR, pword2
            pla
            jmp         SH_FAIL
@usage:
            lda         #<s_umount
            ldy         #>s_umount
            jmp         SH_USAGE

; unmount [new] old: old's mounts and binds, or new's only (UNMOUNT), as rc's
SH_UNMOUNT:
            LDR         IOPTR, pword
            jsr         sh_word
            bcs         @usage
            LDR         IOPTR, pword2
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
            jcc         SH_OK
            pha
            LDR         IOPTR, pword
            pla
            jmp         SH_FAIL
@usage:
            lda         #<s_uunmount
            ldy         #>s_uunmount
            jmp         SH_USAGE

; The command's done: status 0.  OUT: C = 1
SH_OK:
            stz         sh_status
            stz         statbuf
            jmp         SET_STATUS

; The usage at .A/.Y said, status 1.  OUT: C = 1
SH_USAGE:
            jsr         STROUT
            jsr         CRDO
            lda         #1
            sta         sh_status
            stz         statbuf
            jmp         SET_STATUS

s_ubind:    .byte       "usage: bind [-a|-b] [-c] new old", 0
s_umount:   .byte       "usage: mount [-a|-b] [-c] #x old [spec]", 0
s_uunmount: .byte       "usage: unmount [new] old", 0

; newns: the default namespace again (nslib's: Plan 9's newns, init's and rc's)
SH_NEWNS:
            jsr         NEWNS_DO
            jcc         SH_OK
            jmp         SH_FAIL0

; exit: BASIC's end, its code the last rc line's
SH_EXIT:
            lda         sh_status
            jmp         EXIT_CODE

; The namespace made, as newns makes it (nslib's ns_default), its buffers at BANK_WINDOW in a bank taken for the
; while (BASIC's own selected again after).  OUT: C = 0; or C = 1, .A the error (E_NOMEM: no bank to be had)
NEWNS_DO:
            lda         #1
            jsr         BANKS_ALLOC
            bcc         :+
            lda         #E_NOMEM
            rts
:
            pha
            sta         RAM_BANK
            jsr         ns_default
            php
            pha
            lda         bas_bank                            ; (BASIC's bank again: none, if it has none)
            bmi         :+
            sta         RAM_BANK
:
            pla
            plp
            plx                                             ; (The bank given back)
            php
            pha
            txa
            ldx         #1
            jsr         BANKS_FREE
            pla
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
            lda         INPUTBUFFER,x
            cmp         #'-'
            bne         @done
            inx
@letter:
            lda         INPUTBUFFER,x
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

; The next word from INPUTBUFFER,sw_i into IOPTR's buffer (SW_MAX characters), zero-terminated, rc's way: '...'
; quoted ('' a quote), $name the environment's variable (its first word).  OUT: C = 0; or C = 1, none (the line's end)
sh_word:
            ldx         sw_i
            jsr         sw_skipx
            lda         INPUTBUFFER,x
            bne         :+
            stx         sw_i
            sec
            rts
:
            stz         sw_len
@char:
            lda         INPUTBUFFER,x
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
            sta         (IOPTR),y
            clc
            rts
@quoted:
            inx
@q:
            lda         INPUTBUFFER,x
            beq         @end
            inx
            cmp         #$27
            bne         @qput
            lda         INPUTBUFFER,x                       ; ('': a ')
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
            lda         INPUTBUFFER,x
            cmp         #'_'
            beq         :+
            jsr         name_charu
            bcs         @named
            lda         INPUTBUFFER,x                       ; (As it was: the name's case kept)
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
            lda         IOPTR
            adc         sw_len
            sta         r1
            lda         IOPTR+1
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
            sta         (IOPTR),y
            inc         sw_len
:
            rts

; pword = the environment's variable r0's first word
env_pword:
            LDR         r1, pword
            lda         #SW_MAX
; The environment's variable r0's first word into r1's buffer, .A bytes at most, zero-terminated ("" if it isn't
; set).  OUT: .A its length.  Keeps IOPTR
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
            ply                                             ; (r1 back, in KWPTR: the buffer)
            sty         KWPTR+1
            ply
            sty         KWPTR
            bcs         @none
            cpx         #0                                  ; (Its length, at most r2's)
            bne         :+
            cmp         r2
            bcc         :++
:
            lda         r2
:
            sta         sw_skipn
            ldy         #0
:
            cpy         sw_skipn
            beq         @end
            lda         (KWPTR),y
            beq         @end
            iny
            bra         :-
@none:
            ldy         #0
@end:
            lda         #0
            sta         (KWPTR),y
            tya
            rts

.bss
sw_skipn:   .res        1
.code

; ---- basic -l: the login

; The login shell's start (main's, for -l): its namespace (newns), its window's console at /dev (not window 0's:
; #c's taken off it, #c$window put after it, as rc's /lib/profile does), its notes its note group's (/dev/consctl's
; group), the shell's rule on; then /lib/basic/profile.bas run as typed lines (if there's one), and the prompt
LOGIN:
            inc         shell
            jsr         NEWNS_DO
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
            LDR         r1, s_dev
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
            LDR         r1, s_dev
            lda         #MAFTER
            jsr         BIND
@group:
            LDR         r0, s_consctl                       ; echo -n group >/dev/consctl
            lda         #O_WRITE
            jsr         OPEN
            bcs         @profile
            pha
            LDR         r0, s_group
            LDR         r1, 5
            pla
            pha
            jsr         WRITE
            pla
            jsr         CLOSE
@profile:
            LDR         r0, s_profile                       ; The profile, if there's one, as typed
            lda         #O_READ
            jsr         OPEN
            bcs         @prompt
            ldx         #SRC_LOAD
            jsr         SRC_OPENED
            lda         #SRC_LOAD
            sta         cur_src
            inc         ld_exec
@prompt:
            jmp         L2351                               ; (Not RESTART: its I/O reset; no OK in the shell)

s_conc:     .byte       "#c", 0
s_dev:      .byte       "/dev", 0
s_group:    .byte       "group"
s_profile:  .byte       "/lib/basic/profile.bas", 0

; ---- ENV$

; "ENV$" FUNCTION: ENV$(name$): the environment's variable (rc's: $home, $window, $status ...), its first word; ""
; if it isn't set
ENV_FN:
            jsr         CHKSTR
            jsr         FREFAC                              ; (.A its length, INDEX its characters)
            cmp         #32
            bcc         :+
            lda         #31
:
            tay
            lda         #0
            sta         envname,y
:
            dey
            bmi         :+
            lda         (INDEX),y
            sta         envname,y
            bra         :-
:
            LDR         r0, envname
            jsr         env_pword
            jsr         STRSPA                              ; (.A its length: the string's room, at FAC+1)
            tay
:
            dey
            bmi         :+
            lda         pword,y
            sta         (FAC+1),y
            bra         :-
:
            pla                                             ; (A string's: not CHKNUM's, UNARY's return)
            pla
            jmp         PUTNEW

; nslib (the SDK's: newns), its zero page BASIC's temporaries (nothing of BASIC's is live while it runs: a line of
; the shell's, or the login), its buffers at BANK_WINDOW, in a bank of their own meanwhile (NEWNS_DO)
.macro MOVR to, from
            lda         from
            sta         to
            lda         from + 1
            sta         to + 1
.endmacro
NS_ZP       = 1
ns_p        = INDEX
ns_end      = DEST
ns_w        = HIGHDS
ns_line     = HIGHTR
ns_task     = RESULT
ns_n        = RESULT + 1
ns_fl       = RESULT + 2
ns_fd       = RESULT + 3
ns_d        = RESULT + 4
ns_len      = TEMP1
NS_BSS      = BANK_WINDOW
.include "nslib.s"
.assert NS_BSS_SIZE <= BANK_SIZE, error, "nslib's buffers: a bank"
