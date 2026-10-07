; ****************************************************************************
; rc - the shell (docs/design/reimplementation-from-scratch.md, §15.2): Plan 9's rc, small, in assembly, run in place.
;   rc [-l] [-i] [-c command | file [args]]
;   -c command   the command run, and rc ends with its status
;   file args    the file's commands ($0 the file, $* its args), then the end
;   (neither)    the commands typed at fd 0, a prompt before each ($prompt: its first word, or its second for a
;                line that goes on), till its end (Ctrl-D)
;   A note (Ctrl-C) ends what's running, not rc, which waits for it to end as always (it may take the note itself,
;   and go on); then rc goes back to its prompt, or, running a file or -c, ends with its status.
;   -l           first, the default namespace made (newns) and /rom/lib/profile run (a shell's start)
; It starts with the environment's variables and functions (var.s), $task its task, $path (. /bin) and $prompt
; ('% ' and a tab) if it has none.  Its pieces: lex.s (tokens), parse.s (a command line: a tree), word.s (words:
; lists), exec.s (running a tree), var.s (variables, the environment), builtin.s, mem.s (the arena, the heap).
;
; Input comes from a stack of sources: a file's fd (the console's) or a text in memory (-c's, a function's, eval's).
; An fd's characters are copied into the text buffer as they're read, so a command's text (a function's body, a
; pipeline's stage for rc -c) is in memory as long as its tree is.  An error (rc_error: syntax, out of memory ...)
; is said on fd 2 and ends the command: rc goes back to its prompt (or, running a file or -c, ends).

.include "rc.inc"

            HYX2_PROGRAM "rc", main, 2

.zeropage
ap:         .res        2                                   ; The arena's top (mem.s)
ctop:       .res        2                                   ; The text buffer's top
src_at:     .res        2                                   ; Where the last character read is (lex.s's spans)
p0:         .res        2                                   ; Pointers for all (a routine's own: not kept by calls)
p1:         .res        2
p2:         .res        2
p3:         .res        2
sp_:        .res        2                                   ; (A source's pointer)

.bss
src:        .res        1                                   ; The source being read (its index), $FF: none
t0:         .res        1
t1:         .res        1
num:        .res        2                                   ; A number
args:       .res        2                                   ; rc's arguments (TA_ARGS)
interactive: .res       1                                   ; <> 0: a prompt for each command (fd 0's)
interrupted: .res       1                                   ; A note came (the handler's)
prompt1:    .res        1                                   ; <> 0: the next line read is a command's first
abort_sp:   .res        1                                   ; The outermost loop's stack (rc_abort goes back there)
abort_src:  .res        1                                   ;   and its source
text_base:  .res        2                                   ; The text buffer (from the break)
text_end:   .res        2
src_kind:   .res        SRC_MAX                             ; Each source: SRC_FD or SRC_TEXT ...
src_fd:     .res        SRC_MAX                             ;   an fd's ...
src_posl:   .res        SRC_MAX                             ;   the next character ...
src_posh:   .res        SRC_MAX
src_endl:   .res        SRC_MAX                             ;   the end (an fd's: of what's been read) ...
src_endh:   .res        SRC_MAX
src_bufl:   .res        SRC_MAX                             ;   an fd's buffer (in the heap)
src_bufh:   .res        SRC_MAX
obuf:       .res        80                                  ; A line for fd 2
olen:       .res        1
flag_c:     .res        2                                   ; -c's command (its word in the arguments), or 0
flag_l:     .res        1

SRC_FD          = 1
SRC_TEXT        = 2

.code

; ****************************************************************************
main:
            HYX2_BANKS_INIT
            MOVR        args, r0
            lda         #$FF
            sta         src
            stz         interactive
            stz         interrupted
            stz         olen
            jsr         mem_init
            bcc         :+
            LDR         r0, s_nomem
            jsr         PUTS
            lda         #E_NOMEM
            stz         r0
            stz         r0 + 1
            jmp         EXITS
:
            clc                                             ; The text buffer, after them
            lda         heap_base
            adc         #<HEAP_SIZE
            sta         text_base
            sta         ctop
            sta         r0
            lda         heap_base + 1
            adc         #>HEAP_SIZE
            sta         text_base + 1
            sta         ctop + 1
            clc
            adc         #>TEXT_SIZE
            sta         text_end + 1
            sta         r0 + 1
            lda         text_base
            sta         text_end
            jsr         BREAK
            jsr         exec_init
            FAR2        lex_init
            tsx                                             ; (An error before the loop: rc ends)
            stx         abort_sp
            lda         #$FF
            sta         abort_src
            LDR         r0, notes                           ; A note ends what's running, not rc (-c's and a
            jsr         NOTIFY                              ;   file's: rc then ends, at its next command)
            jsr         var_import
            jsr         GETPID                              ; $task
            sta         num
            stz         num + 1
            LDR         p1, obuf
            jsr         rc_number
            tax
            LDR         p2, obuf
            LDR         p1, s_task
            lda         #4
            jsr         var_set_word
            LDR         p1, s_path                          ; $path, $prompt: if they aren't set
            lda         #4
            jsr         var_get
            sta         p0
            stx         p0 + 1
            lda         (p0)
            cmp         #LIST_END
            bne         :+
            LDR         p2, d_path
            LDR         p1, s_path
            lda         #4
            jsr         var_set
:
            LDR         p1, s_prompt
            lda         #6
            jsr         var_get
            sta         p0
            stx         p0 + 1
            lda         (p0)
            cmp         #LIST_END
            bne         :+
            LDR         p2, d_prompt
            LDR         p1, s_prompt
            lda         #6
            jsr         var_set
:

; ---- Its arguments: -c, -l, -i, then a file and its args
            stz         flag_c
            stz         flag_c + 1
            stz         flag_l
            MOVR        p0, args
@arg:
            lda         (p0)
            cmp         #'-'
            bne         @args_done
            ldy         #1
            lda         (p0),Y
            cmp         #'c'
            bne         :+
            jsr         next_arg                            ; -c: the next word
            MOVR        flag_c, p0
            jsr         next_arg
            bra         @arg
:
            cmp         #'l'
            bne         :+
            inc         flag_l
            bra         @next
:
            cmp         #'i'
            bne         @args_done
            inc         interactive
@next:
            jsr         next_arg
            bra         @arg

@args_done:
            lda         flag_l                              ; -l: newns, then /rom/lib/profile
            beq         :+
            PUSHW       p0                                  ; (The arguments' place)
            jsr         ns_default
            jsr         profile
            PULLW       p0
:
            lda         flag_c                              ; -c
            ora         flag_c + 1
            beq         @file
            MOVR        p1, flag_c
            MOVR        p0, flag_c
            jsr         str_len
            ldx         #0
            jsr         run_text
            jmp         rc_exits

@file:
            lda         (p0)                                ; A file: its commands
            beq         @console
            jsr         set_star_file
            MOVR        r0, p0
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            pha
            LDR         r0, s_rc_colon
            jsr         out2s
            MOVR        r0, p0
            jsr         out2s
            pla
            jsr         say_error
            lda         #1
            stz         r0
            stz         r0 + 1
            jmp         EXITS
:
            jsr         run_file_top
            jmp         rc_exits

@console:                                                   ; Fd 0's commands, prompted
            lda         #1
            sta         interactive
            lda         #0
            jsr         run_file_top
            jmp         rc_exits

; p0 on to the next argument (past this one's 0)
next_arg:
            lda         (p0)
            beq         :+
            inc         p0
            bne         next_arg
            inc         p0 + 1
            bra         next_arg
:
            inc         p0
            bne         :+
            inc         p0 + 1
:
            rts

; $0 = the file (p0), $* = the arguments after it
set_star_file:
            PUSHW       p0
            jsr         str_len
            tax
            MOVR        p2, p0
            LDR         p1, s_zero
            lda         #1
            jsr         var_set_word
            PULLW       p0
            PUSHW       p0
            jsr         next_arg
            jsr         list_start                          ; The rest, a list
            pha
            phx
@arg:
            lda         (p0)
            beq         @end
            MOVR        p1, p0
            jsr         str_len
            pha
            jsr         list_append_word
            pla
            jsr         next_arg
            bra         @arg

@end:
            jsr         list_end
            plx
            pla
            sta         p2
            stx         p2 + 1
            LDR         p1, s_star
            lda         #1
            jsr         var_set
            PULLW       p0
            rts

; /rom/lib/profile run, if there's one
profile:
            LDR         r0, s_profile
            lda         #O_READ
            jsr         OPEN
            bcs         :+
            jsr         run_file
:
            rts

; The note handler: a note ends what's running (its tasks get it too), not rc.  In RAM: a note may come while either
; bank is at $A000 (the second parses a line), and the kernel calls the handler with the bank that's there
.pushseg
.segment "DATA"
notes:
            lda         #1
            sta         interrupted
            clc
            rts
.popseg

; ****************************************************************************
; Sources

; The outermost loop: fd .A's commands run (rc_abort comes back here: the arena, the text, the sources and the
; redirections as they were before the command)
run_file_top:
            jsr         src_push_fd
            lda         src
            sta         abort_src
run_file_top_loop:
            tsx
            stx         abort_sp
@loop:
            lda         interrupted                         ; A note ended the command: the prompt on a new line
            beq         :+
            lda         interactive                         ; (Not the console's: the next command ends rc)
            beq         :+
            stz         interrupted
            lda         #LF
            jsr         PUTC
:
            jsr         run_bg_reap
            MOVR        ap, arena_base                      ; (Nothing kept between commands)
            MOVR        ctop, text_base
            lda         #1
            sta         prompt1
            FAR2        parse_line
            bcs         @end
            jsr         run_cmd
            bra         @loop

@end:
            jmp         src_pop

; Fd .A's commands run, here (.: the fd closed at its end)
run_file:
            jsr         src_push_fd
            jsr         lines
            jmp         src_pop

; .A/.X bytes at p1 run as commands (-c's, eval's, a function's)
run_text:
            jsr         src_push_text
            jsr         lines
            jmp         src_pop

; The source's commands, a line at a time, each with the arena and the text back to where they were
lines:
            jsr         arena_mark
            pha
            phx
            lda         ctop
            pha
            lda         ctop + 1
            pha
@line:
            FAR2        parse_line
            bcs         @end
            jsr         run_cmd
            tsx                                             ; (Back to the marks)
            lda         $0104,X
            sta         ap
            lda         $0103,X
            sta         ap + 1
            lda         $0102,X
            sta         ctop
            lda         $0101,X
            sta         ctop + 1
            bra         @line

@end:
            pla
            sta         ctop + 1
            pla
            sta         ctop
            plx
            pla
            jmp         arena_release

; A source: fd .A (its buffer from the heap; moved above the children's fds, unless it's 0)
src_push_fd:
            cmp         #0
            beq         :+
            jsr         high_fd
:
            pha
            jsr         src_new
            pla
            sta         src_fd,X
            lda         #SRC_FD
            sta         src_kind,X
            lda         #<LINE_SIZE
            ldx         #>LINE_SIZE
            jsr         heap_alloc
            ldy         src
            sta         src_bufl,Y
            txa
            sta         src_bufh,Y
            lda         #0                                  ; (Nothing read yet)
            sta         src_posl,Y
            sta         src_endl,Y
            sta         src_posh,Y
            sta         src_endh,Y
            FAR2        lex_init
            rts

; A source: .A/.X bytes at p1
src_push_text:
            pha
            phx
            jsr         src_new
            lda         #SRC_TEXT
            sta         src_kind,X
            lda         p1
            sta         src_posl,X
            lda         p1 + 1
            sta         src_posh,X
            pla
            sta         t0
            pla
            clc
            adc         p1
            sta         src_endl,X
            lda         t0
            adc         p1 + 1
            sta         src_endh,X
            FAR2        lex_init
            rts

; A new source on the stack: .X = it (src)
src_new:
            ldx         src
            inx
            cpx         #SRC_MAX
            bcc         :+
            LDR         r0, s_deep
            jmp         rc_error
:
            stx         src
            rts

; The source done with (an fd's buffer freed, its fd closed, but fd 0)
src_pop:
            ldx         src
            bmi         @done
            lda         src_kind,X
            cmp         #SRC_FD
            bne         @off
            lda         src_bufl,X
            pha
            lda         src_bufh,X
            tax
            pla
            jsr         heap_free
            ldx         src
            lda         src_fd,X
            beq         @off
            jsr         high_close
@off:
            dec         src
            FAR2        lex_init
@done:
            rts

; The next character of the source.  OUT: C = 0, .A = it (src_at: where it is: a text's, or the text buffer's copy
; of an fd's); or C = 1: its end
src_getc:
            ldx         src
            bpl         :+
            sec
            rts
:
            lda         src_posl,X                          ; (sp_: the next)
            sta         sp_
            lda         src_posh,X
            sta         sp_ + 1
            lda         src_kind,X
            cmp         #SRC_TEXT
            bne         @fd
            lda         sp_                                 ; A text: to its end
            cmp         src_endl,X
            lda         sp_ + 1
            sbc         src_endh,X
            bcs         @end
            MOVR        src_at, sp_
            jsr         @step
            lda         (src_at)
            clc
            rts

@end:
            sec
            rts

@fd:
            lda         sp_                                 ; An fd: what's been read, or more
            cmp         src_endl,X
            bne         @have
            lda         sp_ + 1
            cmp         src_endh,X
            bne         @have
            jsr         refill
            bcs         @end
            ldx         src
            lda         src_posl,X
            sta         sp_
            lda         src_posh,X
            sta         sp_ + 1
@have:
            lda         (sp_)                               ; Into the text buffer
            pha
            jsr         @step
            lda         ctop                                ; (Full: the command's too long)
            cmp         text_end
            lda         ctop + 1
            sbc         text_end + 1
            bcc         :+
            pla
            LDR         r0, s_long
            jmp         rc_error
:
            pla
            sta         (ctop)
            MOVR        src_at, ctop
            inc         ctop
            bne         :+
            inc         ctop + 1
:
            lda         (src_at)
            clc
            rts

@step:                                                      ; The next one's place
            ldx         src
            inc         src_posl,X
            bne         :+
            inc         src_posh,X
:
            rts

; Source .X's fd read again (the prompt first, if it's fd 0 and rc is interactive).  OUT: C = 0; or C = 1: its end
refill:
            lda         interactive
            beq         @read
            lda         src_fd,X
            bne         @read
            jsr         prompt
            ldx         src
@read:
            lda         src_bufl,X
            sta         r0
            sta         src_posl,X
            lda         src_bufh,X
            sta         r0 + 1
            sta         src_posh,X
            LDR         r1, LINE_SIZE
            lda         src_fd,X
            jsr         READ
            bcc         :+
            cmp         #E_INTR                             ; Ctrl-C at the prompt: a new one
            bne         @eof
            lda         interactive
            beq         @eof
            lda         #LF
            jsr         PUTC
            jmp         rc_abort
:
            stx         t1                                  ; (The count: .A/.X)
            ldx         src
            clc
            adc         src_posl,X
            sta         src_endl,X
            lda         t1
            adc         src_posh,X
            sta         src_endh,X
            lda         src_endl,X                          ; Nothing: the end
            cmp         src_posl,X
            bne         :+
            lda         src_endh,X
            cmp         src_posh,X
            beq         @eof
:
            clc
            rts

@eof:
            sec
            rts

; The prompt: $prompt's first word (a command's first line), or its second (one that goes on)
prompt:
            LDR         p1, s_prompt
            lda         #6
            jsr         var_get
            sta         p0
            stx         p0 + 1
            lda         prompt1
            stz         prompt1
            bne         :+
            lda         (p0)                                ; (The second)
            cmp         #LIST_END
            beq         @done
            lda         p0
            ldx         p0 + 1
            jsr         list_next
            sta         p0
            stx         p0 + 1
:
            lda         (p0)
            cmp         #LIST_END
            beq         @done
            tax
            beq         @done
            ldy         #1
:
            lda         (p0),Y
            phx
            phy
            jsr         PUTC
            ply
            plx
            iny
            dex
            bne         :-
@done:
            rts

; ****************************************************************************
; Errors

; Error r0 (a message): said ("rc: it"), $status it, and the command ended (rc_abort)
rc_error:
            MOVR        p0, r0
            jsr         rc_say
            lda         in_error                            ; (Not again, from setting $status)
            bne         rc_abort
            inc         in_error
            jsr         str_len
            tax
            MOVR        p2, p0
            LDR         p1, s_status
            lda         #6
            jsr         var_set_word
            ; (Falls into rc_abort)

; Back to the outermost loop (its stack, its source; the redirections and saved values undone); rc ends if that
; isn't the console's
rc_abort:
            stz         interrupted
            stz         in_error
            ldx         abort_sp
            txs
            jsr         redir_restore_all
            lda         #0
            jsr         var_local_restore
@pop:
            lda         src
            cmp         abort_src
            beq         :+
            jsr         src_pop
            bra         @pop
:
            lda         interactive
            beq         :+
            lda         abort_src
            bmi         :+
            FAR2        lex_init
            jmp         run_file_top_again
:
            jmp         rc_exits

; (rc_abort's way back into the loop: its source still there)
run_file_top_again:
            ldx         src
            lda         #0                                  ; (What's left of the line: dropped)
            sta         src_posl,X
            sta         src_endl,X
            sta         src_posh,X
            sta         src_endh,X
            jmp         run_file_top_loop

; "rc: " and r0, on fd 2, and a new line
rc_say:
            PUSHW       r0
            LDR         r0, s_rc_colon
            jsr         out2s
            PULLW       r0
; r0 on fd 2, and a new line
rc_say2:
            jsr         out2s
            lda         #LF
            jmp         out2

; The error .A's text on fd 2, and a new line
say_error:
            pha
            LDR         r0, s_colon
            jsr         out2s
            LDR         r0, obuf_err
            pla
            jsr         ERRSTR
            LDR         r0, obuf_err
            jmp         rc_say2

; The string r0 on fd 2
out2s:
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            phy
            jsr         out2
            ply
            iny
            bne         :-
:
            rts

; .A on fd 2 (a line at a time: written at its new line, or when the buffer's full; fd 2 closed: the bring-up
; console's, by PUTS).  Keeps .X, .Y
out2:
            phx
            phy
            jsr         @put
            ply
            plx
            rts

@put:
            ldx         olen
            sta         obuf,X
            inc         olen
            cmp         #LF
            beq         @flush
            cpx         #78
            bcc         @done
@flush:
            LDR         r0, obuf
            lda         olen
            sta         ow_n
            sta         r1
            stz         r1 + 1
            stz         olen
            lda         #2
            jsr         WRITE
            bcc         @done
            ldx         ow_n                                ; (Fd 2 closed: PUTS, the string ended)
            stz         obuf,X
            LDR         r0, obuf
            jsr         PUTS
@done:
            rts

; ****************************************************************************
; The pieces

; num in decimal into p1's buffer: .A = its digits' count
rc_number:
            ldy         #0
            lda         num
            ora         num + 1
            bne         @digits
            lda         #'0'
            sta         (p1)
            lda         #1
            rts

@digits:
            lda         num + 1                             ; Divide by 10, the digits pushed (last first)
            pha
            lda         num
            pha
            ldx         #0
@div:
            stz         t0                                  ; num / 10 (t0: the remainder)
            ldy         #16
:
            asl         num
            rol         num + 1
            rol         t0
            lda         t0
            cmp         #10
            bcc         :+
            sbc         #10
            sta         t0
            inc         num
:
            dey
            bne         :--
            lda         t0
            ora         #'0'
            pha
            inx
            lda         num
            ora         num + 1
            bne         @div
            txa
            sta         t0
            ldy         #0
:
            pla
            sta         (p1),Y
            iny
            dex
            bne         :-
            pla                                             ; (num back)
            sta         num
            pla
            sta         num + 1
            lda         t0
            rts

; C = 0 if $status is true: no words, or each empty or 0
rc_true:
            LDR         p1, s_status
            lda         #6
            jsr         var_get
            sta         p0
            stx         p0 + 1
@word:
            lda         (p0)
            cmp         #LIST_END
            beq         @yes
            tax
            beq         @next
            cmp         #1
            bne         @no
            ldy         #1
            lda         (p0),Y
            cmp         #'0'
            bne         @no
@next:
            lda         p0
            ldx         p0 + 1
            jsr         list_next
            sta         p0
            stx         p0 + 1
            bra         @word

@yes:
            clc
            rts

@no:
            sec
            rts

.rodata
s_rc_colon: .byte       "rc: ", 0
s_colon:    .byte       ": ", 0
s_nomem:    .byte       "rc: no memory", CR, LF, 0
s_deep:     .byte       "too deep", 0
s_long:     .byte       "command too long", 0
s_task:     .byte       "task"
s_path:     .byte       "path"
s_prompt:   .byte       "prompt"
s_status:   .byte       "status"
s_star:     .byte       "*"
s_zero:     .byte       "0"
s_profile:  .byte       "/rom/lib/profile", 0
d_path:     .byte       1, ".", 4, "/bin", LIST_END
d_prompt:   .byte       2, "% ", 1, 9, LIST_END

.bss
obuf_err:   .res        32
in_error:   .res        1
ow_n:       .res        1
