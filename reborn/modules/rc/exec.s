; ****************************************************************************
; exec.s - running the tree.  run_cmd runs a command node and sets $status (a program's: its exit message, or its
; code in decimal, or none for 0; true is none, or 0).
;   Redirections change rc's own fds for the command (the old ones kept above CHILD_FDS, put back after), so a
;   program, a built-in, a function and a { } all see them the same way; a program gets rc's fds 0 to
;   CHILD_FDS - 1 (SPAWN_FDMAP), and none of rc's own above them.
;   A program's name without a / is looked for in each of $path's directories: SPAWN of dir/name, the next one on
;   E_NOENT.  Before it starts, the variables go to the environment (var.s).
;   A pipeline's stages, a background command and `{}'s command run as programs: a simple command's program
;   itself, anything else (a built-in, a function, a { }, an if ...) rc -c with its text.
;   Background tasks are kept (their records waited for as they end: run_bg_reap, before each prompt).

.include "rc.inc"

.zeropage
xn:         .res        2                                   ; A node
xw:         .res        2                                   ; A list (a command's words)
xs:         .res        2                                   ; A text's start ...

.bss
xe:         .res        2                                   ;   and end
xt:         .res        1                                   ; A task
xf:         .res        1                                   ; SPAWN's flags
xin:        .res        1                                   ; A pipeline stage's input fd ($FF: rc's own) ...
xout:       .res        1                                   ;   its output fd ...
xoutn:      .res        1                                   ;   and which of its fds that is
if_failed:  .res        1                                   ; <> 0: the last if's condition was false (if not)
fn_depth:   .res        1
fd_save_fd: .res        SAVE_MAX                            ; Redirections' saved fds: each fd ...
fd_save_to: .res        SAVE_MAX                            ;   and where it went ($FF: it was closed)
fd_save_depth: .res     1
high_used:  .res        16                                  ; rc's fds from RC_FD_HIGH: <> 0 in use
pipe_tasks: .res        16                                  ; A pipeline's stages' tasks
pipe_n:     .res        1
bg_tasks:   .res        16                                  ; Background tasks not yet waited for ($FF: none)
msgbuf:     .res        32                                  ; An exit message
pathbuf:    .res        PATH_MAX + 1
fdmap:      .res        CHILD_FDS + 1                       ; SPAWN's map: rc's fds 0 to CHILD_FDS - 1

.code

; Node xn's word at .Y: .A/.X
.macro FIELD    off
            ldy         #off
            jsr         field
.endmacro

; The map, and nothing saved or running in the background (rc.s, as it starts)
exec_init:
            ldx         #CHILD_FDS - 1
:
            txa
            sta         fdmap + 1,X
            dex
            bpl         :-
            lda         #CHILD_FDS
            sta         fdmap
            ldx         #15
            lda         #$FF
:
            sta         bg_tasks,X
            stz         high_used,X
            dex
            bpl         :-
            stz         fd_save_depth
            stz         fn_depth
            rts

; ****************************************************************************
; The command node .A/.X run (0: nothing)
run_cmd:
            sta         xn
            stx         xn + 1
            ora         xn + 1
            beq         @done
            lda         interrupted                         ; (A note came: back to the prompt)
            beq         :+
            jmp         rc_abort
:
            lda         (xn)
            asl
            tax
            jmp         (run_vec - 2,X)

@done:
            rts

; xn's word at .Y: .A/.X
field:
            lda         (xn),Y
            pha
            iny
            lda         (xn),Y
            tax
            pla
            rts

; C_SEQ: each in turn
c_seq:
            PUSHW       xn
@cmd:
            FIELD       1
            jsr         run_cmd
            tsx                                             ; (The node: under us)
            lda         $0102,X
            sta         xn
            lda         $0101,X
            sta         xn + 1
            FIELD       3
            sta         xn
            stx         xn + 1
            tsx
            lda         xn
            sta         $0102,X
            lda         xn + 1
            sta         $0101,X
            ora         xn
            bne         @cmd
            pla
            pla
            rts

; C_AND, C_OR: the second if the first was true (&&) or false (||)
c_and:
            clc
            bra         andor

c_or:
            sec
andor:
            php
            PUSHW       xn
            FIELD       1
            jsr         run_cmd
            PULLW       xn
            jsr         rc_true                             ; (C = 0: true)
            ror         a                                   ; (Bit 7: false)
            plp
            ror         a                                   ; (Bit 7: ||; bit 6: false)
            and         #$C0
            beq         @second                             ; (&&, true)
            cmp         #$C0
            beq         @second                             ; (||, false)
            rts

@second:
            FIELD       3
            jmp         run_cmd

; C_NOT: the command, its status turned over
c_not:
            FIELD       1
            jsr         run_cmd
            jsr         rc_true
            bcs         :+
            LDR         p1, s_one                           ; True: false now
            lda         #1
            jmp         status_word
:
            lda         #0                                  ; False: true now
            jmp         status_word

; C_BRACE: its body, with its redirections
c_brace:
            lda         fd_save_depth
            pha
            PUSHW       xn
            FIELD       3
            jsr         redir_apply
            PULLW       xn
            bcs         @failed
            FIELD       1
            jsr         run_cmd
@failed:
            pla
            jmp         redir_restore

; C_IF: the command, if the condition is true
c_if:
            PUSHW       xn
            FIELD       1
            jsr         run_cmd
            PULLW       xn
            jsr         rc_true
            lda         #1
            sta         if_failed
            bcs         @done
            stz         if_failed
            FIELD       3
            jsr         run_cmd
            stz         if_failed
@done:
            rts

; C_IFNOT: the command, if the last if's condition was false
c_ifnot:
            lda         if_failed
            beq         :+
            stz         if_failed
            FIELD       1
            jmp         run_cmd
:
            rts

; C_WHILE: the body while the condition is true (the arena back to here each time)
c_while:
            jsr         arena_mark
            pha
            phx
            PUSHW       xn
@loop:
            tsx
            lda         $0102,X
            sta         xn
            lda         $0101,X
            sta         xn + 1
            FIELD       1
            jsr         run_cmd
            jsr         rc_true
            bcs         @done
            tsx
            lda         $0102,X
            sta         xn
            lda         $0101,X
            sta         xn + 1
            FIELD       3
            jsr         run_cmd
            tsx
            lda         $0104,X
            pha
            lda         $0103,X
            tax
            pla
            jsr         arena_release
            bra         @loop

@done:
            PULLW       xn
            plx
            pla
            jsr         arena_release
            lda         #0                                  ; (Its status: true)
            jmp         status_word

; C_FOR: the body once for each word of the list (or $*), the variable set to it
c_for:
            ldy         #7
            lda         (xn),Y
            bne         @list
            LDR         p1, s_star
            lda         #1
            jsr         var_get
            jsr         list_dup
            bra         @go

@list:
            PUSHW       xn
            FIELD       3
            jsr         expand_list
            sta         xw
            stx         xw + 1
            PULLW       xn
            lda         xw
            ldx         xw + 1
@go:
            sta         xw
            stx         xw + 1
            PUSHW       xn
            FIELD       1                                   ; The name
            jsr         expand_one
            sta         xs                                  ; (xs: the name's list)
            stx         xs + 1
            PULLW       xn
            jsr         arena_mark
            pha
            phx
            PUSHW       xn
            PUSHW       xs
@word:
            lda         (xw)
            cmp         #LIST_END
            beq         @done
            tsx                                             ; The variable = the word
            lda         $0102,X
            sta         xs
            lda         $0101,X
            sta         xs + 1
            clc
            lda         xs
            adc         #1
            sta         p1
            lda         xs + 1
            adc         #0
            sta         p1 + 1
            PUSHW       xw
            clc
            lda         xw
            adc         #1
            sta         p2
            lda         xw + 1
            adc         #0
            sta         p2 + 1
            lda         (xw)
            tax
            lda         (xs)
            jsr         var_set_word
            tsx                                             ; The body
            lda         $0106,X
            sta         xn
            lda         $0105,X
            sta         xn + 1
            FIELD       5
            jsr         run_cmd
            PULLW       xw
            tsx
            lda         $0106,X
            pha
            lda         $0105,X
            tax
            pla
            jsr         arena_release
            lda         xw
            ldx         xw + 1
            jsr         list_next
            sta         xw
            stx         xw + 1
            bra         @word

@done:
            pla
            pla
            pla
            pla
            plx
            pla
            jmp         arena_release

; C_SWITCH: the commands after the first case whose patterns match the subject, to the next case
c_switch:
            PUSHW       xn
            FIELD       1
            jsr         expand_one
            sta         xs                                  ; (xs: the subject)
            stx         xs + 1
            PULLW       xn
            FIELD       3                                   ; The body: its C_SEQ list
            sta         xn
            stx         xn + 1
@find:
            lda         xn
            ora         xn + 1
            beq         @done
            FIELD       1                                   ; A case?
            sta         p0
            stx         p0 + 1
            lda         (p0)
            cmp         #C_CASE
            bne         @skip
            PUSHW       xn
            PUSHW       xs
            ldy         #1
            lda         (p0),Y
            tax
            iny
            lda         (p0),Y
            pha
            txa
            plx
            jsr         expand_raw_list                     ; (Its patterns: markers kept)
            sta         xw
            stx         xw + 1
            PULLW       xs
            jsr         any_match
            PULLW       xn
            bcc         @run
@skip:
            FIELD       3
            sta         xn
            stx         xn + 1
            bra         @find

@run:                                                       ; The commands after it, to the next case
            FIELD       3
            sta         xn
            stx         xn + 1
            ora         xn + 1
            beq         @done
            FIELD       1
            sta         p0
            stx         p0 + 1
            lda         (p0)
            cmp         #C_CASE
            beq         @done
            PUSHW       xn
            lda         p0
            ldx         p0 + 1
            jsr         run_cmd
            PULLW       xn
            bra         @run

@done:
            rts

; C_CASE outside a switch: nothing
c_case:
            rts

; C_MATCH: ~ subject patterns: true if a word of the subject matches one of the patterns
c_match:
            PUSHW       xn
            FIELD       1
            jsr         expand_one
            sta         xs
            stx         xs + 1
            PULLW       xn
            PUSHW       xs
            FIELD       3
            jsr         expand_raw_list
            sta         xw
            stx         xw + 1
            PULLW       xs
            jsr         any_match
            lda         #0
            bcc         :+
            LDR         p1, s_one
            lda         #1
:
            jmp         status_word

; C = 0 if a word of list xs matches a pattern of list xw
any_match:
            MOVR        p3, xs
@subject:
            lda         (p3)
            cmp         #LIST_END
            beq         @no
            MOVR        p0, xw
@pattern:
            lda         (p0)
            cmp         #LIST_END
            beq         @nextsub
            clc
            lda         p0
            adc         #1
            sta         p1
            lda         p0 + 1
            adc         #0
            sta         p1 + 1
            clc
            lda         p3
            adc         #1
            sta         p2
            lda         p3 + 1
            adc         #0
            sta         p2 + 1
            PUSHW       p0
            PUSHW       p3
            lda         (p3)
            tax
            lda         (p0)
            jsr         match
            PULLW       p3
            PULLW       p0
            bcc         @yes
            lda         p0
            ldx         p0 + 1
            jsr         list_next
            sta         p0
            stx         p0 + 1
            bra         @pattern

@nextsub:
            lda         p3
            ldx         p3 + 1
            jsr         list_next
            sta         p3
            stx         p3 + 1
            bra         @subject

@no:
            sec
@yes:
            rts

; C_FN: each name a function, the body's text
c_fn:
            PUSHW       xn
            FIELD       1
            jsr         expand_one_list
            sta         xw
            stx         xw + 1
            PULLW       xn
            FIELD       3                                   ; p2: the text; xs: its length
            sta         p2
            stx         p2 + 1
            FIELD       5
            sec
            sbc         p2
            sta         xs
            txa
            sbc         p2 + 1
            sta         xs + 1
@name:
            lda         (xw)
            cmp         #LIST_END
            beq         @done
            clc
            lda         xw
            adc         #1
            sta         p1
            lda         xw + 1
            adc         #0
            sta         p1 + 1
            PUSHW       p2
            lda         (xw)
            ldx         xs
            ldy         xs + 1
            jsr         fn_set
            PULLW       p2
            lda         xw
            ldx         xw + 1
            jsr         list_next
            sta         xw
            stx         xw + 1
            bra         @name

@done:
            lda         #0
            jmp         status_word

; C_FNDEL: each name's function removed
c_fndel:
            FIELD       1
            jsr         expand_one_list
            sta         xw
            stx         xw + 1
@name:
            lda         (xw)
            cmp         #LIST_END
            beq         @done
            clc
            lda         xw
            adc         #1
            sta         p1
            lda         xw + 1
            adc         #0
            sta         p1 + 1
            lda         (xw)
            jsr         fn_del
            lda         xw
            ldx         xw + 1
            jsr         list_next
            sta         xw
            stx         xw + 1
            bra         @name

@done:
            lda         #0
            jmp         status_word

; C_BG: the command as a program, in a note group of its own, not waited for: $apid its task
c_bg:
            FIELD       3
            sta         xs
            stx         xs + 1
            FIELD       5
            sta         xe
            stx         xe + 1
            FIELD       1
            ldy         #SPAWN_NEWGROUP
            sty         xf
            jsr         spawn_stage
            bcs         @failed
            sta         xt
            ldx         #15                                 ; Kept, to be waited for
:
            lda         bg_tasks,X
            cmp         #$FF
            beq         :+
            dex
            bpl         :-
            bra         @apid
:
            lda         xt
            sta         bg_tasks,X
@apid:
            lda         xt
            sta         num
            stz         num + 1
            LDR         p1, msgbuf
            jsr         rc_number
            tax
            LDR         p2, msgbuf
            LDR         p1, s_apid
            lda         #4
            jsr         var_set_word
            lda         #0
            jmp         status_word

@failed:
            tax
            beq         :+
            jmp         status_error
:
            rts

; ****************************************************************************
; C_SIMPLE: its words expanded; none: its assignments made (and its redirections done); else, its assignments for
; it alone, its redirections, then the function, built-in or program its first word names
c_simple:
            jsr         arena_mark
            pha
            phx
            lda         fd_save_depth
            pha
            ldy         #1                                  ; Words?  (Then its assignments are for it alone: made
            lda         (xn),Y                              ;   first, so its words see them)
            iny
            ora         (xn),Y
            bne         @command
            PUSHW       xn                                  ; No words: the assignments, for good
            FIELD       5
            jsr         assign
            PULLW       xn
            FIELD       3
            jsr         redir_apply
            lda         #0
            bcs         @end
            jsr         status_word
            bra         @end

@command:
            jsr         var_local_mark                      ; Its assignments: for it alone
            pha
            PUSHW       xn
            FIELD       5
            jsr         local_set
            PULLW       xn
            PUSHW       xn                                  ; Its words
            FIELD       1
            jsr         expand_list
            sta         xw
            stx         xw + 1
            PULLW       xn
            lda         (xw)                                ; (None, after all: nothing to run)
            cmp         #LIST_END
            sec
            beq         @locals
            PUSHW       xw
            PUSHW       xn
            FIELD       3
            jsr         redir_apply
            PULLW       xn
            PULLW       xw
            bcs         @locals
            jsr         run_words                           ; (xw: the words)
@locals:
            pla
            jsr         var_local_restore
@end:
            pla                                             ; (Its redirections undone)
            jsr         redir_restore
            plx
            pla
            jmp         arena_release

; The words xw run: a function, a built-in, or a program (waited for)
run_words:
            lda         xw                                  ; A function?
            clc
            adc         #1
            sta         p1
            lda         xw + 1
            adc         #0
            sta         p1 + 1
            lda         (xw)
            jsr         var_fn
            bcs         :+
            jmp         call_fn
:
            lda         (xw)                                ; (Its name's length again)
            jsr         builtin_find
            bcs         :+
            jmp         builtin_run
:
            lda         xw
            ldx         xw + 1
            stz         xf
            jsr         spawn_path
            bcs         status_error2
            jmp         wait_child

; (status_error, for a branch)
status_error2:
            jmp         status_error

; Assignments (a list: +0 the next, +2 the name, +4 the value) made: each variable set
assign:
            sta         xn
            stx         xn + 1
@one:
            lda         xn
            ora         xn + 1
            beq         @done
            PUSHW       xn
            FIELD       4                                   ; The value: a list, globbed
            jsr         expand_word_list
            sta         p2
            stx         p2 + 1
            PUSHW       p2
            tsx
            lda         $0104,X
            sta         xn
            lda         $0103,X
            sta         xn + 1
            FIELD       2                                   ; The name
            jsr         expand_one
            sta         p0
            stx         p0 + 1
            PULLW       p2
            clc
            lda         p0
            adc         #1
            sta         p1
            lda         p0 + 1
            adc         #0
            sta         p1 + 1
            lda         (p0)
            jsr         var_set
            PULLW       xn
            FIELD       0
            sta         xn
            stx         xn + 1
            bra         @one

@done:
            rts

; The word node .A/.X expanded and globbed (an assignment's value): .A/.X
expand_word_list:
            pha                                             ; (As a word list of one: a cell on the stack's
            phx                                             ;   copy, in the arena)
            lda         #4
            ldx         #0
            jsr         arena_alloc
            sta         p0
            stx         p0 + 1
            ldy         #3
            pla
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            lda         #0
            dey
            sta         (p0),Y
            dey
            sta         (p0),Y
            lda         p0
            ldx         p0 + 1
            jmp         expand_list

; Assignments for one command (the list .A/.X): each variable's value kept (var.s: var_local_save, undone by
; var_local_restore back to a mark), and set
local_set:
            sta         xn
            stx         xn + 1
@one:
            lda         xn
            ora         xn + 1
            beq         @done
            PUSHW       xn
            FIELD       4                                   ; The value
            jsr         expand_word_list
            sta         p2
            stx         p2 + 1
            tsx
            lda         $0102,X
            sta         xn
            lda         $0101,X
            sta         xn + 1
            PUSHW       p2
            FIELD       2                                   ; The name
            jsr         expand_one
            sta         p0
            stx         p0 + 1
            clc
            lda         p0
            adc         #1
            sta         p1
            lda         p0 + 1
            adc         #0
            sta         p1 + 1
            lda         (p0)
            jsr         var_local_save
            PULLW       p2
            jsr         var_local_set
            PULLW       xn
            FIELD       0
            sta         xn
            stx         xn + 1
            bra         @one

@done:
            rts

; ****************************************************************************
; Programs

; The words .A/.X run as a program (SPAWN with xf's flags, and the map): its name without a /, from each of
; $path's directories.  OUT: C = 0, .A = its task; or C = 1, .A = the error (E_NOENT: not found)
spawn_path:
            sta         xw
            stx         xw + 1
            jsr         var_export
            jsr         args_block                          ; (p2: its arguments)
            ldy         #1                                  ; A / in it, or a . first?  As it is
            lda         (xw),Y
            cmp         #'.'
            beq         @as_is
            lda         (xw)
            tax
:
            lda         (xw),Y
            cmp         #'/'
            beq         @as_is
            iny
            dex
            bne         :-
            LDR         p1, s_path                          ; Each of $path's
            lda         #4
            jsr         var_get
            sta         p3
            stx         p3 + 1
@dir:
            lda         (p3)
            cmp         #LIST_END
            beq         @noent
            ldx         #0                                  ; dir/name
            tay
            beq         :++
            ldy         #1
:
            lda         (p3),Y
            jsr         pput
            iny
            tya
            dec         a
            cmp         (p3)
            bne         :-
:
            lda         #'/'
            jsr         pput
            jsr         name_put
            bcs         @next
            jsr         spawn_it
            bcc         @done
            cmp         #E_NOENT
            bne         @fail
@next:
            lda         p3
            ldx         p3 + 1
            jsr         list_next
            sta         p3
            stx         p3 + 1
            bra         @dir

@as_is:
            ldx         #0
            jsr         name_put
            bcs         @noent
            jmp         spawn_it

@noent:
            lda         #E_NOENT
@fail:
            sec
@done:
            rts

; The name (xw's first word) into pathbuf from .X, ended.  OUT: C = 1: too long
name_put:
            lda         (xw)
            beq         :++
            ldy         #1
:
            lda         (xw),Y
            jsr         pput
            iny
            tya
            dec         a
            cmp         (xw)
            bne         :-
:
            cpx         #PATH_MAX
            bcs         @long
            stz         pathbuf,X
            clc
            rts

@long:
            sec
            rts

; .A into pathbuf at .X (past PATH_MAX: dropped; name_put says so).  Keeps .Y
pput:
            cpx         #PATH_MAX
            bcs         :+
            sta         pathbuf,X
            inx
:
            rts

; SPAWN of pathbuf, its arguments p2, xf's flags and the map.  OUT: C = 0, .A = the task; or C = 1, .A
spawn_it:
            LDR         r0, pathbuf
            MOVR        r1, p2
            LDR         r2, fdmap
            lda         xf
            ora         #SPAWN_FDMAP
            jmp         SPAWN

; p2: the words of xw after the first, as SPAWN's arguments (each with its 0; a 0 after them), in the arena
args_block:
            jsr         arena_mark
            sta         p2
            stx         p2 + 1
            lda         xw
            ldx         xw + 1
            jsr         list_next                           ; (Past the name)
            sta         p0
            stx         p0 + 1
            ldx         #0                                  ; (.X: the bytes, ARGS_MAX at most)
@word:
            lda         (p0)
            cmp         #LIST_END
            beq         @end
            tay
            beq         @zero
            ldy         #1
:
            lda         (p0),Y
            jsr         aput
            iny
            tya
            dec         a
            cmp         (p0)
            bne         :-
@zero:
            lda         #0
            jsr         aput
            lda         p0
            phx
            ldx         p0 + 1
            jsr         list_next
            sta         p0
            stx         p0 + 1
            plx
            bra         @word

@end:
            lda         #0                                  ; (The empty one that ends them)
; .A onto the arguments (a byte of the arena), counted in .X (ARGS_MAX at most: an error past it).  Keeps .Y
aput:
            phx
            phy
            pha
            lda         #1
            ldx         #0
            jsr         arena_alloc
            sta         p1
            stx         p1 + 1
            pla
            sta         (p1)
            ply
            plx
            inx
            cpx         #ARGS_MAX + 1
            bcs         @big
            rts

@big:
            LDR         r0, s_args
            jmp         rc_error

; The child .A waited for: $status its exit (a note ending the wait: waited for again)
wait_child:
            sta         xt
@wait:
            LDR         r0, msgbuf
            lda         xt
            jsr         WAIT
            bcc         :+
            cmp         #E_INTR
            beq         @wait
            jmp         status_error
:
            lda         msgbuf                              ; Its message, or its code
            beq         @code
            LDR         p0, msgbuf
            LDR         p1, msgbuf
            jsr         str_len                             ; (After the LDRs: they'd lose .A)
            jmp         status_word

@code:
            txa
            beq         status_word                         ; (0: true, no status)
            sta         num
            stz         num + 1
            LDR         p1, msgbuf
            jsr         rc_number
            jmp         status_word

; $status = the word .A bytes at p1 (none: .A = 0: the empty list)
status_word:
            tax
            beq         @empty
            MOVR        p2, p1
            LDR         p1, s_status
            lda         #6
            jmp         var_set_word

@empty:
            LDR         p2, empty
            LDR         p1, s_status
            lda         #6
            jmp         var_set

; $status = error .A's text (and said, on fd 2: "rc: name: text")
status_error:
            pha
            jsr         say_name
            LDR         r0, msgbuf
            pla
            jsr         ERRSTR
            LDR         r0, msgbuf
            jsr         rc_say2
            LDR         p0, msgbuf
            LDR         p1, msgbuf
            jsr         str_len
            jmp         status_word

; "rc: " and xw's first word and ": ", on fd 2
say_name:
            LDR         r0, s_rc_colon
            jsr         out2s
            lda         (xw)
            beq         :++
            ldy         #1
:
            lda         (xw),Y
            jsr         out2
            iny
            tya
            dec         a
            cmp         (xw)
            bne         :-
:
            LDR         r0, s_colon
            jmp         out2s

; ****************************************************************************
; Pipelines, background commands, backquotes: commands as programs

; A stage: the command node .A/.X (its text xs to xe) started as a program, with xf's flags.  A simple command's
; first word a program: that program (its assignments and redirections for it); anything else: rc -c and its text.
; OUT: C = 0, .A = its task; or C = 1, .A = the error (0: one already said)
spawn_stage:
            sta         xn
            stx         xn + 1
            lda         (xn)
            cmp         #C_SIMPLE
            beq         :+
            jmp         spawn_rc
:
            jsr         arena_mark
            pha
            phx
            lda         fd_save_depth
            pha
            jsr         var_local_mark
            pha
            PUSHW       xn
            FIELD       1
            jsr         expand_list
            sta         xw
            stx         xw + 1
            PULLW       xn
            lda         (xw)                                ; (No words: rc does it)
            cmp         #LIST_END
            beq         @undo
            clc                                             ; A function or a built-in: rc does it
            lda         xw
            adc         #1
            sta         p1
            lda         xw + 1
            adc         #0
            sta         p1 + 1
            lda         (xw)
            jsr         var_fn
            bcc         @undo
            lda         (xw)
            jsr         builtin_find
            bcc         @undo
            PUSHW       xw                                  ; A program
            PUSHW       xn
            FIELD       5
            jsr         local_set
            PULLW       xn
            FIELD       3
            jsr         redir_apply
            PULLW       xw
            lda         #0
            bcs         @back
            lda         xw
            ldx         xw + 1
            jsr         spawn_path
@back:
            sta         ss_a                                ; (Its task, or its error, and C: kept)
            php
            pla
            sta         ss_p
            pla
            jsr         var_local_restore
            pla
            jsr         redir_restore
            plx
            pla
            jsr         arena_release
            lda         ss_p
            pha
            lda         ss_a
            plp
            rts

@undo:
            pla                                             ; (Nothing run here)
            pla
            plx
            pla
            jsr         arena_release
            ; (Falls into spawn_rc)

; The command text xs to xe run as rc -c TEXT (with xf's flags).  OUT: C = 0, .A = the task; or C = 1, .A
spawn_rc:
            jsr         var_export
            jsr         arena_mark                          ; Its arguments: -c, the text
            sta         p2
            stx         p2 + 1
            lda         #'-'
            jsr         aput0
            lda         #'c'
            jsr         aput0
            lda         #0
            jsr         aput0
            MOVR        p0, xs
@text:
            lda         p0
            cmp         xe
            lda         p0 + 1
            sbc         xe + 1
            bcs         @end
            lda         (p0)
            jsr         aput0
            inc         p0
            bne         @text
            inc         p0 + 1
            bra         @text

@end:
            lda         #0
            jsr         aput0
            lda         #0
            jsr         aput0
            LDR         r0, s_rcpath
            MOVR        r1, p2
            LDR         r2, fdmap
            lda         xf
            ora         #SPAWN_FDMAP
            jmp         SPAWN

; .A appended to the arena (an argument block's byte: ARGS_MAX at most, counted from p2)
aput0:
            pha
            sec                                             ; (Room?)
            lda         ap
            sbc         p2
            cmp         #ARGS_MAX
            bcs         @big
            lda         #1
            ldx         #0
            jsr         arena_alloc
            sta         p1
            stx         p1 + 1
            pla
            sta         (p1)
            rts

@big:
            LDR         r0, s_args
            jmp         rc_error

; C_PIPE: each stage started (left to right, each's output the next one's input), then each waited for:
; $status the last one's
c_pipe:
            stz         pipe_n
            lda         #$FF
            sta         xin
            sta         xout
            lda         xn
            ldx         xn + 1
            jsr         pipe_start
            ldx         #0                                  ; Each waited for, in turn
@wait:
            cpx         pipe_n
            beq         @done
            phx
            lda         pipe_tasks,X
            jsr         wait_child
            plx
            inx
            bra         @wait

@done:
            rts

; The pipe node .A/.X's stages started: its input xin, its output xout (as its fd xoutn)
pipe_start:
            sta         xn
            stx         xn + 1
            lda         (xn)
            cmp         #C_PIPE
            beq         @pipe
            jmp         stage                               ; (Not a pipe: the text was set by the caller)

@pipe:
            jsr         PIPE                                ; A pipe: its ends kept above the children's fds
            bcc         :+
            jmp         status_error
:
            phx
            jsr         high_fd                             ; (.A: its read end, moved)
            sta         t1
            pla
            jsr         high_fd
            pha                                             ; (The write end)
            lda         t1
            pha                                             ; (The read end)
            PUSHW       xn
            lda         xout                                ; (The right's output: this one's)
            pha
            lda         xoutn
            pha
            lda         xin                                 ; The left: our input, into the write end (as its fd)
            pha
            tsx
            lda         $0107,X                             ; (The write end)
            sta         xout
            ldy         #5
            lda         (xn),Y
            sta         xoutn
            FIELD       6                                   ; (Its text)
            sta         xs
            stx         xs + 1
            FIELD       8
            sta         xe
            stx         xe + 1
            FIELD       1
            jsr         pipe_start
            pla                                             ; (xin back)
            sta         xin
            tsx                                             ; The write end closed (the left has it)
            lda         $0106,X
            jsr         high_close
            pla                                             ; The right: from the read end, into our output
            sta         xoutn
            pla
            sta         xout
            PULLW       xn
            pla                                             ; (The read end)
            sta         xin
            pha
            FIELD       10
            sta         xs
            stx         xs + 1
            FIELD       12
            sta         xe
            stx         xe + 1
            FIELD       3
            jsr         pipe_start
            pla
            jsr         high_close                          ; (The read end: the right has it)
            pla                                             ; (The write end: closed above)
            rts

; One stage (xn) started: rc's fd 0 its input xin, its fd xoutn its output xout, for the moment
stage:
            lda         fd_save_depth
            pha
            lda         xin
            cmp         #$FF
            beq         :+
            ldx         #0
            jsr         redir_dup                           ; (Fd 0 = xin, the old one saved)
:
            lda         xout
            cmp         #$FF
            beq         :+
            ldx         xoutn
            jsr         redir_dup
:
            stz         xf
            lda         xn
            ldx         xn + 1
            jsr         spawn_stage
            tax
            pla
            php
            phx
            jsr         redir_restore
            plx
            plp
            bcs         @failed
            txa
            ldx         pipe_n
            sta         pipe_tasks,X
            inc         pipe_n
            rts

@failed:
            txa
            beq         :+
            jmp         status_error
:
            rts

; `{cmd}: its output read (wa: in the arena, num its bytes): the command (node .A/.X: W_BQ) started with fd 1 a
; pipe's write end, the pipe read to its end, the command waited for
run_capture:
            sta         xn
            stx         xn + 1
            FIELD       3
            sta         xs
            stx         xs + 1
            FIELD       5
            sta         xe
            stx         xe + 1
            jsr         PIPE
            bcc         :+
            jmp         status_error
:
            phx
            jsr         high_fd
            sta         t1
            pla
            jsr         high_fd                             ; (The write end)
            pha
            lda         t1
            pha
            lda         fd_save_depth
            pha
            tsx
            lda         $0103,X
            ldx         #1
            jsr         redir_dup
            stz         xf
            FIELD       1
            jsr         spawn_stage
            sta         xt
            php
            pla
            sta         ss_p                                ; (Its C)
            pla
            jsr         redir_restore
            pla                                             ; (The read end)
            sta         rd_fd
            pla                                             ; The write end closed: the end comes as the child's
            jsr         high_close                          ;   goes
            lda         ss_p
            pha
            plp
            bcc         :+
            lda         rd_fd
            jsr         high_close
            lda         xt
            beq         @said
            jmp         status_error

@said:
            stz         num                                 ; (Nothing read)
            stz         num + 1
            MOVR        wa, ap
            rts
:
            jsr         arena_mark                          ; Read to its end, into the arena
            sta         wa_out
            stx         wa_out + 1
            stz         num
            stz         num + 1
            lda         rd_fd
            pha
@read:
            lda         #<256
            ldx         #>256
            jsr         arena_alloc
            sta         r0
            stx         r0 + 1
            LDR         r1, 256
            tsx
            lda         $0101,X
            jsr         READ
            bcc         :+
            lda         #0                                  ; (An error, a note: the end)
            tax
:
            sta         rd_n                                ; (The bytes it gave: the rest of the 256 back)
            stx         rd_n + 1
            sec
            lda         ap
            sbc         #<256
            sta         ap
            lda         ap + 1
            sbc         #>256
            sta         ap + 1
            clc
            lda         ap
            adc         rd_n
            sta         ap
            lda         ap + 1
            adc         rd_n + 1
            sta         ap + 1
            clc
            lda         num
            adc         rd_n
            sta         num
            lda         num + 1
            adc         rd_n + 1
            sta         num + 1
            lda         rd_n
            ora         rd_n + 1
            bne         @read
            pla
            jsr         high_close
            PUSHW       num                                 ; (Kept: wait_child's status may use it)
            lda         xt
            jsr         wait_child
            PULLW       num
            MOVR        wa, wa_out
            rts

; ****************************************************************************
; Functions

; The function vr called: its body's text parsed and run, $* the words after its name (xw) meanwhile
call_fn:
            lda         fn_depth
            cmp         #FN_DEPTH_MAX
            bcc         :+
            LDR         r0, s_deep
            jmp         rc_error
:
            inc         fn_depth
            ldy         #3                                  ; Its text: p0 (its length, then it)
            lda         (vr),Y
            sta         p0
            iny
            lda         (vr),Y
            sta         p0 + 1
            jsr         var_local_mark
            pha
            PUSHW       p0
            LDR         p1, s_star                          ; $*: the words after the name, for it
            lda         #1
            jsr         var_local_save
            lda         xw
            ldx         xw + 1
            jsr         list_next
            sta         p2
            stx         p2 + 1
            jsr         var_local_set
            PULLW       p0
            ldy         #1
            lda         (p0),Y
            tax
            lda         (p0)
            pha
            clc
            lda         p0
            adc         #2
            sta         p1
            lda         p0 + 1
            adc         #0
            sta         p1 + 1
            pla
            jsr         run_text                            ; (rc.s: .A/.X bytes at p1)
            pla
            jsr         var_local_restore
            dec         fn_depth
            rts

; ****************************************************************************
; Redirections

; The redirection list .A/.X done, in the order they were written (the list has them last first), rc's old fds
; kept (redir_restore puts them back).  OUT: C = 0; or C = 1: one failed (said; $status set)
redir_apply:
            sta         p0
            stx         p0 + 1
            ora         p0 + 1
            bne         :+
            clc
            rts
:
            PUSHW       p0
            ldy         #0                                  ; The ones before it first
            lda         (p0),Y
            tax
            iny
            lda         (p0),Y
            pha
            txa
            plx
            jsr         redir_apply
            PULLW       p0
            bcc         :+
            rts
:
            ldy         #2
            lda         (p0),Y
            sta         t0                                  ; (Its type)
            iny
            lda         (p0),Y
            sta         t1                                  ; (Its fd)
            lda         t0
            cmp         #RD_DUP
            bne         :+
            ldy         #4                                  ; >[n=m]
            lda         (p0),Y
            ldx         t1
            jmp         redir_dup
:
            cmp         #RD_CLOSE
            bne         @file
            lda         t1                                  ; >[n=]
            jsr         save_fd
            lda         t1
            jsr         CLOSE
            clc
            rts

@file:
            lda         t1
            pha
            lda         t0
            pha
            ldy         #5                                  ; Its file: one word
            lda         (p0),Y
            tax
            iny
            lda         (p0),Y
            pha
            txa
            plx
            jsr         expand_one
            sta         xw
            stx         xw + 1
            lda         (xw)
            cmp         #LIST_END
            beq         @bad
            jsr         list_next_x                         ; (One word only)
            lda         (p0)
            cmp         #LIST_END
            bne         @bad
            ldx         #0                                  ; Its name, into pathbuf
            jsr         name_put
            bcs         @bad
            pla
            sta         t0
            pla
            sta         t1
            LDR         r0, pathbuf
            lda         t0
            cmp         #RD_IN
            bne         :+
            lda         #O_READ
            jsr         OPEN
            bra         @opened
:
            lda         #O_WRITE                            ; > and >>: made if it isn't there
            ldx         t0
            cpx         #RD_OUT
            bne         :+
            ora         #O_TRUNC
:
            jsr         OPEN
            bcc         @append
            cmp         #E_NOENT
            bne         @opened
            LDR         r0, pathbuf
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            bra         @opened

@append:
            pha                                             ; >>: from its end
            ldx         t0
            cpx         #RD_APPEND
            bne         :+
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #2
            jsr         SEEK
:
            pla
            clc
@opened:
            bcs         @failed
            pha                                             ; Fd t1 = it (the old one saved)
            ldx         t1
            jsr         redir_dup
            pla
            jsr         CLOSE
            clc
            rts

@bad:
            pla
            pla
            LDR         r0, s_redir
            jsr         rc_say
            LDR         p1, s_redir
            lda         #15
            jsr         status_word
            sec
            rts

@failed:
            jsr         status_error
            sec
            rts

; p0 = the word after xw's first
list_next_x:
            lda         xw
            ldx         xw + 1
            jsr         list_next
            sta         p0
            stx         p0 + 1
            rts

; Fd .X = fd .A (rc's old fd .X saved first)
redir_dup:
            pha
            txa
            pha
            jsr         save_fd
            pla
            tax
            pla
            jsr         DUP2
            clc
            rts

; Fd .A saved (moved above the children's fds; closed: noted so): for redir_restore
save_fd:
            ldx         fd_save_depth
            cpx         #SAVE_MAX
            bcs         @many
            sta         fd_save_fd,X
            pha
            jsr         DUP                                 ; (A copy: kept up high)
            bcc         :+
            pla
            ldx         fd_save_depth
            lda         #$FF
            sta         fd_save_to,X
            inc         fd_save_depth
            rts
:
            jsr         high_fd
            ldx         fd_save_depth
            sta         fd_save_to,X
            inc         fd_save_depth
            pla
            rts

@many:
            LDR         r0, s_fds
            jmp         rc_error

; The redirections back to depth .A: each fd as it was
redir_restore:
            sta         t0
@one:
            lda         fd_save_depth
            cmp         t0
            beq         @done
            bcc         @done
            dec         fd_save_depth
            ldx         fd_save_depth
            lda         fd_save_to,X
            cmp         #$FF
            beq         @closed
            pha
            lda         fd_save_fd,X
            tax
            pla
            pha
            jsr         DUP2
            pla
            jsr         high_close
            bra         @one

@closed:
            lda         fd_save_fd,X
            jsr         CLOSE
            bra         @one

@done:
            rts

; Every redirection undone (rc_abort's)
redir_restore_all:
            lda         #0
            jmp         redir_restore

; Fd .A moved above the children's fds (RC_FD_HIGH on).  OUT: .A = where it is now
high_fd:
            sta         hf_fd
            ldx         #RC_FD_HIGH
:
            lda         high_used,X
            beq         :+
            inx
            cpx         #FD_MAX
            bne         :-
            LDR         r0, s_fds
            jmp         rc_error
:
            inc         high_used,X
            phx
            lda         hf_fd
            jsr         DUP2
            lda         hf_fd
            jsr         CLOSE
            pla
            rts

; rc's high fd .A closed (its slot free)
high_close:
            tax
            stz         high_used,X
            jmp         CLOSE

; ****************************************************************************
; Background tasks: those that have ended, waited for (their records: else the tasks wait for good)
run_bg_reap:
            ldx         #15
@task:
            lda         bg_tasks,X
            cmp         #$FF
            beq         @next
            stx         bg_x
            pha
            LDR         r0, tinfo
            pla
            jsr         TASKINFO
            ldx         bg_x
            lda         tinfo + TI_STATE                    ; (Ended: free, its record waiting)
            bne         @next
            lda         bg_tasks,X
            stz         r0
            stz         r0 + 1
            jsr         WAIT
            ldx         bg_x
            lda         #$FF
            sta         bg_tasks,X
@next:
            dex
            bpl         @task
            rts

; The background tasks waited for: .A = one ($FF: all of them)
wait_bg:
            sta         bg_which
            ldx         #15
@task:
            lda         bg_tasks,X
            cmp         #$FF
            beq         @next
            ldy         bg_which
            cpy         #$FF
            beq         :+
            cmp         bg_which
            bne         @next
:
            stx         bg_x
            jsr         wait_child
            ldx         bg_x
            lda         #$FF
            sta         bg_tasks,X
@next:
            dex
            bpl         @task
            rts

; rc ended: its $status as its exit (code 0 if it's true; else 1, and it as the message)
rc_exits:
            jsr         rc_true
            bcs         :+
            stz         r0
            stz         r0 + 1
            lda         #0
            jmp         EXITS
:
            LDR         p1, s_status
            lda         #6
            jsr         var_get
            jsr         list_flat
            sta         p0
            stx         p0 + 1
            lda         (p0)                                ; (Zero-terminated: in msgbuf, 31 at most)
            cmp         #31
            bcc         :+
            lda         #31
:
            tax
            stz         msgbuf,X
            tay
            beq         :++
:
            lda         (p0),Y
            dey
            sta         msgbuf,Y
            bne         :-
:
            LDR         r0, msgbuf
            lda         #1
            jmp         EXITS

.bss
wa_out:     .res        2
tinfo:      .res        TI_SIZE
ss_a:       .res        1                                   ; (spawn_stage's result, a moment)
ss_p:       .res        1
rd_fd:      .res        1                                   ; (run_capture's read end ...
rd_n:       .res        2                                   ;   and a read's count)
hf_fd:      .res        1
bg_x:       .res        1
bg_which:   .res        1
FN_DEPTH_MAX = 8

.rodata
run_vec:    .word       c_simple, c_brace, c_seq, c_bg, c_and, c_or, c_not, c_pipe, c_if, c_ifnot, c_for
            .word       c_while, c_switch, c_case, c_match, c_fn, c_fndel
s_star:     .byte       "*"
s_path:     .byte       "path"
s_status:   .byte       "status"
s_apid:     .byte       "apid"
s_one:      .byte       "1"
empty:      .byte       LIST_END
s_colon:    .byte       ": ", 0
s_rcpath:   .byte       "#m/rc", 0
s_args:     .byte       "arguments too long", 0
s_fds:      .byte       "too many fds", 0
s_redir:    .byte       "bad redirection", 0
s_deep:     .byte       "too deep", 0
