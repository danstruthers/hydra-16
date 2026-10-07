; ****************************************************************************
; ed [file] - a line editor, in the manner of Unix's ed: the old system's (os_rom/shell/edit.s), a program now.
; The text is in the task's RAM, from its break to the top (some 29K), a line ending in LF each; a file's CR LF
; and CR line ends are LFs here.  At its prompt (*), a command, with line numbers (n, or $: the last) before it or
; after it:
;   p [a[,b]]   print lines a to b, numbered (p alone: all of them)
;   a [n]       add lines after line n (a alone: after the last), typed until a line of just "."
;   i [n]       insert lines before line n (i alone: before the first), until "."
;   c a[,b]     change lines a to b: they go, and the lines typed until "." take their place
;   d a[,b]     delete lines a to b
;   w [file]    write the text to the file (or to another one: then that's the file)
;   q           quit (a second q quits without writing what's changed); Q quits at once
;   h           help
; Its commands and lines come from fd 0: the console's lines, edited there; or a file (edit f <script).  The end
; of them quits, as Q does.  Ctrl-C (while it prints, or waits for lines) comes back to the prompt.  What goes
; wrong is said after a ?, and it goes on.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "ed", main

LINE_MAX        = 255           ; A line typed: its bytes at most (the rest left out)

.zeropage
p:          .res        2                                   ; A place in the text
s:          .res        2                                   ; A move's source ...
d:          .res        2                                   ;   destination ...
c:          .res        2                                   ;   and count
n:          .res        2                                   ; A number

.bss
name:       .res        PATH_MAX + 1                        ; The file's name ("": none yet)
line:       .res        LINE_MAX + 1                        ; A line typed (zero-terminated, no LF)
ibuf:       .res        256                                 ; fd 0, read ahead ...
ilen:       .res        1                                   ;   its bytes ...
ipos:       .res        1                                   ;   and how many have been taken
why:        .res        32                                  ; An error's text
text:       .res        2                                   ; The text: its start (the break as it started) ...
tmax:       .res        2                                   ;   the room's end ...
top:        .res        2                                   ;   and its end (past its last line's LF)
lines:      .res        2                                   ; Its lines (count)
first:      .res        2                                   ; A command's lines: the first ...
last:       .res        2                                   ;   and the last ...
given:      .res        1                                   ;   how many numbers were typed (0-2)
cmd:        .res        1                                   ;   and its letter
dirty:      .res        1                                   ; <> 0: changed since it was read or written
quitting:   .res        1                                   ; <> 0: q said, with changes not written
intr:       .res        1                                   ; <> 0: Ctrl-C (the note handler's)
fd:         .res        1                                   ; The file's fd
loop_sp:    .res        1                                   ; The stack at the prompt (abort's)

.code
main:
            jsr         tl_start
            lda         (tl_arg)                            ; The file's name, if there's one
            beq         @named
            ldy         #0
:
            lda         (tl_arg),Y
            sta         name,Y
            beq         :+
            iny
            cpy         #PATH_MAX + 1
            bne         :-
            lda         #E_NAMETOOLONG
            MOVR        r0, tl_arg
            jsr         tl_err
            jmp         tl_end
:
            jsr         tl_next                             ; (One at most)
            beq         @named
            jmp         tl_badusage

@named:
            LDR         r0, 0                               ; The text: from the break ...
            jsr         BREAK
            MOVR        text, r0
            LDR         r0, $8000                           ;   to the top of the task's RAM ($7F00 in task F)
            jsr         BREAK
            bcc         :+
            LDR         r0, $7F00
            jsr         BREAK
            bcc         :+
            MOVR        r0, text
:
            MOVR        tmax, r0
            LDR         r0, notes                           ; Ctrl-C: back to the prompt
            jsr         NOTIFY
            stz         dirty
            stz         quitting
            stz         ilen
            stz         ipos
            jsr         readfile                            ; The file (or a new one)
            tsx                                             ; (The stack at the prompt: abort's)
            stx         loop_sp

loop:
            stz         intr
            lda         #'*'
            jsr         tl_putc
            jsr         getline
            bcs         @end                                ; (The end of the input: as Q)
            jsr         command
            bcc         loop
            jmp         tl_end

@end:
            lda         intr
            bne         abort
            jmp         tl_end

; Ctrl-C came: "?", and the prompt again (the stack as it was there)
abort:
            ldx         loop_sp
            txs
            stz         tl_olen                             ; (What was waiting to be printed: dropped)
            jsr         tl_nl
            lda         #'?'
            jsr         tl_putc
            jsr         tl_nl
            bra         loop

; The note handler: Ctrl-C (the interrupt note) is noted, and the task goes on (a read ends with E_INTR, p stops:
; then abort).  Any other note: its default
notes:
            cmp         #NOTE_INTERRUPT
            bne         :+
            sta         intr
            clc
            rts
:
            sec
            rts

; ****************************************************************************
; Commands

; The command in line.  OUT: C = 1: quit
command:
            jsr         count                               ; (lines: $ and the defaults)
            ldy         #0
            jsr         range                               ; Numbers first (2,4p), or after (p 2,4)
            jsr         skip
            lda         line,Y
            bne         :+
            clc                                             ; (Nothing: nothing to do)
            rts
:
            sta         cmd
            iny
            cmp         #'w'                                ; (w: a name after it, not numbers)
            beq         :+
            lda         given
            bne         :+
            jsr         range
:
            lda         cmd
            cmp         #'q'
            beq         @quit
            stz         quitting                            ; (Anything else: q asks again)
            cmp         #'Q'
            beq         @done
            ldx         #CMDS - 1
:
            cmp         cmd_letters,X
            beq         @run
            dex
            bpl         :-
            LDR         r0, s_what
            jmp         fail

@run:
            txa
            asl
            tax
            jmp         (cmd_routines,X)

@quit:
            lda         dirty                               ; Changes not written: the second q quits
            beq         @done
            lda         quitting
            bne         @done
            inc         quitting
            LDR         r0, s_unsaved
            jmp         fail

@done:
            sec
            rts

; h: the commands (in two: tl_puts takes 255 bytes at most)
c_help:
            LDR         r0, s_help
            jsr         tl_puts
            LDR         r0, s_help2
            jsr         tl_puts
            clc
            rts

; p: print lines first to last (none given: all of them), each numbered
c_print:
            lda         given
            bne         :+
            lda         #1                                  ; (All)
            sta         first
            stz         first + 1
            MOVR        last, lines
            lda         lines
            ora         lines + 1
            bne         :+
            clc                                             ; (No lines at all)
            rts
:
            jsr         check
            bcs         fail
            lda         first
            ldx         first + 1
            jsr         start                               ; p = line first
@line:
            lda         intr                                ; (Ctrl-C: no more)
            beq         :+
            jmp         abort
:
            lda         first                               ; Its number, in 4 places, a space ...
            sta         tl_num
            lda         first + 1
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            lda         #4
            jsr         tl_dec
            jsr         tl_space
@char:                                                      ; ... and its text, to its LF
            lda         (p)
            jsr         next
            jsr         tl_putc
            cmp         #LF
            bne         @char
            lda         first                               ; The next, to last
            cmp         last
            bne         :+
            lda         first + 1
            cmp         last + 1
            beq         @done
:
            inc         first
            bne         @line
            inc         first + 1
            bra         @line

@done:
            clc
            rts

; Say what's wrong (the text at r0, after a ?), and go on.  OUT: C = 0
fail:
            lda         #'?'
            jsr         tl_putc
            jsr         tl_space
            jsr         tl_puts
            jsr         tl_nl
            clc
            rts

; d: delete lines first to last
c_delete:
            jsr         need
            bcs         fail
            jsr         delete
            clc
            rts

; c: change lines first to last: they go, and the lines typed take their place
c_change:
            jsr         need
            bcs         fail
            jsr         delete                              ; (p: where they were)
            jmp         insert

; a: add lines after line first (none: after the last)
c_add:
            lda         given
            bne         :+
            MOVR        first, lines
:
            lda         first                               ; (0: before the first)
            ora         first + 1
            beq         @after
            MOVR        last, first                         ; (A line that's there)
            jsr         check
            bcs         fail
@after:
            inc         first                               ; After it: where the next one starts
            bne         :+
            inc         first + 1
:
            lda         first
            ldx         first + 1
            jsr         start
            jmp         insert

; i: insert lines before line first (none: before the first)
c_insert:
            lda         given
            bne         :+
            lda         #1
            sta         first
            stz         first + 1
:
            lda         first                               ; (1 to the lines + 1: that's after the last)
            ora         first + 1
            beq         @bad
            lda         lines
            clc
            adc         #1
            sta         n
            lda         lines + 1
            adc         #0
            cmp         first + 1
            bcc         @bad
            bne         :+
            lda         n
            cmp         first
            bcc         @bad
:
            lda         first
            ldx         first + 1
            jsr         start
            jmp         insert

@bad:
            LDR         r0, s_noline
            jmp         fail

; w: write the text to the file (a name after w: that file, and it's the file from now on)
c_write:
            jsr         skip
            lda         line,Y
            beq         @named
            ldx         #0                                  ; A name: the file's from now on
:
            lda         line,Y
            beq         :+
            cmp         #' '
            beq         :+
            sta         name,X
            inx
            iny
            cpx         #PATH_MAX
            bne         :-
:
            stz         name,X
@named:
            lda         name
            bne         :+
            LDR         r0, s_noname
            jmp         fail
:
            jsr         writefile
            clc
            rts

; ****************************************************************************
; Lines

; A range: numbers from .Y in line (n, or $: the last line; a, or a,b).  OUT: given = how many (0-2), first and
; last; .Y after them
range:
            stz         given
            jsr         number
            bcs         @done
            inc         given
            lda         n
            sta         first
            sta         last
            lda         n + 1
            sta         first + 1
            sta         last + 1
            jsr         skip
            lda         line,Y
            cmp         #','
            bne         @done
            iny
            inc         given
            jsr         number                              ; (a, alone: a to the last)
            bcc         :+
            MOVR        n, lines
:
            MOVR        last, n
@done:
            rts

; A number from .Y in line (spaces before it skipped): digits, or $ (the last line).  OUT: C = 0: n = it, .Y
; after it; or C = 1: none there
number:
            jsr         skip
            lda         line,Y
            cmp         #'$'
            bne         :+
            iny
            MOVR        n, lines
            clc
            rts
:
            jsr         @digit
            bcs         @none
            stz         n
            stz         n + 1
@more:
            pha                                             ; n * 10 + the digit
            asl         n
            rol         n + 1
            lda         n
            ldx         n + 1
            asl         n
            rol         n + 1
            asl         n
            rol         n + 1
            clc
            adc         n
            sta         n
            txa
            adc         n + 1
            sta         n + 1
            pla
            clc
            adc         n
            sta         n
            bcc         :+
            inc         n + 1
:
            iny
            lda         line,Y
            jsr         @digit
            bcc         @more
            clc
            rts

@none:
            sec
            rts

@digit:                                                     ; .A = a digit's value, C = 0; or C = 1
            sec
            sbc         #'0'
            cmp         #10
            rts

; .Y on past spaces in line
skip:
            lda         line,Y
            cmp         #' '
            bne         :+
            iny
            bra         skip
:
            rts

; Lines first to last, which must be given.  OUT: as check
need:
            lda         given
            bne         check
            LDR         r0, s_which
            sec
            rts

; Are lines first to last there (1 <= first <= last <= the last line)?  OUT: C = 0; or C = 1, r0 = what's wrong
check:
            lda         first
            ora         first + 1
            beq         @bad
            lda         last + 1                            ; first <= last
            cmp         first + 1
            bcc         @bad
            bne         :+
            lda         last
            cmp         first
            bcc         @bad
:
            lda         lines + 1                           ; last <= the last line
            cmp         last + 1
            bcc         @bad
            bne         :+
            lda         lines
            cmp         last
            bcc         @bad
:
            clc
            rts

@bad:
            LDR         r0, s_noline
            sec
            rts

; lines = the text's lines (its LFs)
count:
            stz         lines
            stz         lines + 1
            MOVR        p, text
@byte:
            jsr         at_top
            bcs         @done
            lda         (p)
            jsr         next
            cmp         #LF
            bne         @byte
            inc         lines
            bne         @byte
            inc         lines + 1
            bra         @byte

@done:
            rts

; p = where line .A/.X starts (1: the text's start; the lines + 1: its end)
start:
            sta         n
            stx         n + 1
            MOVR        p, text
@line:
            lda         n                                   ; Past n - 1 LFs
            bne         :+
            dec         n + 1
:
            dec         n
            lda         n
            ora         n + 1
            beq         @done
:
            jsr         at_top
            bcs         @done
            lda         (p)
            jsr         next
            cmp         #LF
            bne         :-
            bra         @line

@done:
            rts

; p on by 1.  Keeps .A
next:
            inc         p
            bne         :+
            inc         p + 1
:
            rts

; Is p at the text's end (top)?  OUT: C = 1: it is
at_top:
            lda         p
            cmp         top
            lda         p + 1
            sbc         top + 1
            rts

; Delete lines first to last (checked): p = where they started
delete:
            lda         last                                ; s = where the line after last starts
            ldx         last + 1
            clc
            adc         #1
            bcc         :+
            inx
:
            jsr         start
            MOVR        s, p
            lda         first                               ; p = d = where first starts
            ldx         first + 1
            jsr         start
            MOVR        d, p
            sec                                             ; c = the rest of the text, after them
            lda         top
            sbc         s
            sta         c
            lda         top + 1
            sbc         s + 1
            sta         c + 1
            clc                                             ; The text's end: where they started + the rest
            lda         d
            adc         c
            sta         top
            lda         d + 1
            adc         c + 1
            sta         top + 1
            jsr         move_down
            lda         #1
            sta         dirty
            rts

; Insert the lines typed next (until a line of just ".", or the end of the input) at p
insert:
            jsr         getline
            bcc         @line
            lda         intr                                ; (Ctrl-C: the prompt; the end of the input: the
            bne         @abort                              ;   prompt too, where it comes again and quits)
@done:
            clc
            rts

@abort:
            jmp         abort

@line:
            lda         line                                ; "."?
            cmp         #'.'
            bne         :+
            lda         line + 1
            beq         @done
:
            ldy         #$FF                                ; .Y = its length; c = its bytes (with the LF)
:
            iny
            lda         line,Y
            bne         :-
            phy
            iny
            sty         c
            stz         c + 1
            clc                                             ; Room?  top + c <= tmax
            lda         top
            adc         c
            tax
            lda         top + 1
            adc         #0
            cmp         tmax + 1
            bcc         @room
            bne         @full
            cpx         tmax
            bcc         @room
            beq         @room
@full:
            ply
            LDR         r0, s_full
            jsr         fail
            bra         insert                              ; (The rest typed: left out, to the .)

@room:
            MOVR        s, p                                ; The text from p on moves up by c: s = p, d =
            clc                                             ;   p + c, the count = top - p
            lda         p
            adc         c
            sta         d
            lda         p + 1
            adc         #0
            sta         d + 1
            lda         c                                   ; (Kept: the move counts c down)
            pha
            sec
            lda         top
            sbc         p
            sta         c
            lda         top + 1
            sbc         p + 1
            sta         c + 1
            jsr         move_up
            pla                                             ; The text's end moves up by it
            clc
            adc         top
            sta         top
            bcc         :+
            inc         top + 1
:
            ply                                             ; The line, and its LF, at p
            lda         #LF
            sta         (p),Y
:
            cpy         #0
            beq         :+
            dey
            lda         line,Y
            sta         (p),Y
            bra         :-
:                                                           ; p past it
            lda         (p)
            jsr         next
            cmp         #LF
            bne         :-
            lda         #1
            sta         dirty
            jmp         insert

; Copy c bytes from s to d, upwards (d <= s)
move_down:
            lda         c
            ora         c + 1
            beq         @done
            lda         (s)
            sta         (d)
            inc         s
            bne         :+
            inc         s + 1
:
            inc         d
            bne         :+
            inc         d + 1
:
            lda         c
            bne         :+
            dec         c + 1
:
            dec         c
            bra         move_down

@done:
            rts

; Copy c bytes from s to d, from the top down (d >= s)
move_up:
            clc                                             ; From their ends
            lda         s
            adc         c
            sta         s
            lda         s + 1
            adc         c + 1
            sta         s + 1
            clc
            lda         d
            adc         c
            sta         d
            lda         d + 1
            adc         c + 1
            sta         d + 1
@byte:
            lda         c
            ora         c + 1
            beq         @done
            lda         s
            bne         :+
            dec         s + 1
:
            dec         s
            lda         d
            bne         :+
            dec         d + 1
:
            dec         d
            lda         (s)
            sta         (d)
            lda         c
            bne         :+
            dec         c + 1
:
            dec         c
            bra         @byte

@done:
            rts

; ****************************************************************************
; The file

; Read the file (name) into the text, its line ends made LFs, and say how many lines it has ("name: 3 lines"); or,
; if it isn't there, that it's a new one
readfile:
            MOVR        top, text                           ; (Nothing yet)
            lda         name
            bne         :+
            rts                                             ; (No name: w needs one)
:
            LDR         r0, name
            lda         #O_READ
            jsr         OPEN
            bcc         @open
            cmp         #E_NOENT
            beq         :+
            jmp         file_error
:
            jsr         say_name
            LDR         r0, s_new
            jsr         tl_puts
            jmp         tl_nl

@open:
            sta         fd
            MOVR        r0, text                            ; All of it (to 1 byte short of the room: a last
            sec                                             ;   LF may have to be added)
            lda         tmax
            sbc         text
            sta         r1
            lda         tmax + 1
            sbc         text + 1
            sta         r1 + 1
            lda         r1
            bne         :+
            dec         r1 + 1
:
            dec         r1
            MOVR        c, r1                               ; (c: what was asked for)
            lda         fd
            jsr         READ
            php
            pha
            stx         n + 1                               ; (n: what was read)
            lda         fd
            jsr         CLOSE
            pla
            plp
            bcs         @error
            sta         n
            MOVR        s, text                             ; Its line ends: LFs (s reads, d writes, to p)
            MOVR        d, text
            clc
            lda         text
            adc         n
            sta         p
            lda         text + 1
            adc         n + 1
            sta         p + 1
@byte:
            lda         s
            cmp         p
            lda         s + 1
            sbc         p + 1
            bcs         @read
            lda         (s)
            inc         s
            bne         :+
            inc         s + 1
:
            cmp         #CR
            bne         @put
            lda         s                                   ; A CR: an LF; and the LF of a CR LF goes
            cmp         p
            lda         s + 1
            sbc         p + 1
            bcs         @lf
            lda         (s)
            cmp         #LF
            bne         @lf
            inc         s
            bne         @lf
            inc         s + 1
@lf:
            lda         #LF
@put:
            sta         (d)
            inc         d
            bne         @byte
            inc         d + 1
            bra         @byte

@error:
            jmp         file_error

@read:
            lda         d                                   ; A last line with no line end: one
            cmp         text
            bne         :+
            lda         d + 1
            cmp         text + 1
            beq         @end                                ; (Empty)
:
            lda         d
            bne         :+
            dec         d + 1
:
            dec         d
            lda         (d)
            inc         d
            bne         :+
            inc         d + 1
:
            cmp         #LF
            beq         @end
            lda         #LF
            sta         (d)
            inc         d
            bne         @end
            inc         d + 1
@end:
            MOVR        top, d
            lda         n                                   ; All of it?  (Read to the room's end: maybe not)
            cmp         c
            bne         :+
            lda         n + 1
            cmp         c + 1
            bne         :+
            LDR         r0, s_big
            jsr         fail
:
            jsr         count                               ; "name: n lines"
            jsr         say_name
            MOVR        tl_num, lines
            stz         tl_num + 2
            stz         tl_num + 3
            lda         #0
            jsr         tl_dec
            LDR         r0, s_lines
            jsr         tl_puts
            jmp         tl_nl

; Write the text to the file (name), emptied first, and say how many bytes ("name: 120 bytes")
writefile:
            LDR         r0, name
            lda         #O_WRITE | O_TRUNC                  ; (One that's there, emptied; else a new one)
            jsr         OPEN
            bcc         @open
            cmp         #E_NOENT
            bne         file_error
            LDR         r0, name
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            bcs         file_error
@open:
            sta         fd
            MOVR        r0, text
            sec
            lda         top
            sbc         text
            sta         r1
            sta         n
            lda         top + 1
            sbc         text + 1
            sta         r1 + 1
            sta         n + 1
            ora         r1
            beq         :+
            lda         fd
            jsr         WRITE
            bcs         @fail
:
            lda         fd
            jsr         CLOSE
            bcs         file_error
            stz         dirty
            jsr         say_name
            MOVR        tl_num, n
            stz         tl_num + 2
            stz         tl_num + 3
            lda         #0
            jsr         tl_dec
            LDR         r0, s_bytes
            jsr         tl_puts
            jmp         tl_nl

@fail:
            pha
            lda         fd
            jsr         CLOSE
            pla
            ; Fall through

; Error .A about the file: "? name: why"
file_error:
            LDR         r0, why
            jsr         ERRSTR
            lda         #'?'
            jsr         tl_putc
            jsr         tl_space
            jsr         say_name
            LDR         r0, why
            jsr         tl_puts
            jmp         tl_nl

; "name: " into the output
say_name:
            LDR         r0, name
            jsr         tl_puts
            lda         #':'
            jsr         tl_putc
            jmp         tl_space

; ****************************************************************************
; Input

; A line from fd 0 into line (zero-terminated, its LF left out; 255 bytes at most, the rest left out; a CR before
; its LF left out too).  What's waiting to be printed goes first (a prompt).  OUT: C = 0; or C = 1: the end of the
; input, or Ctrl-C (intr)
getline:
            jsr         tl_flush
            ldy         #0
@byte:
            ldx         ipos
            cpx         ilen
            bcc         @have
            phy                                             ; (Empty: read more)
            LDR         r0, ibuf
            LDR         r1, 255
            lda         #0
            jsr         READ
            ply
            stz         ipos
            bcs         @fail
            sta         ilen
            cmp         #0
            bne         @byte
            cpy         #0                                  ; (The end: a last line with no LF first)
            bne         @end
            sec
            rts

@have:
            lda         ibuf,X
            inc         ipos
            cmp         #LF
            beq         @end
            cmp         #CR
            beq         @byte
            cpy         #LINE_MAX
            bcs         @byte
            sta         line,Y
            iny
            bra         @byte

@end:
            lda         #0
            sta         line,Y
            clc
            rts

@fail:
            stz         ilen                                ; (Ctrl-C: E_INTR, intr set; anything else: the end)
            sec
            rts

; ****************************************************************************
; Commands and texts

.rodata
cmd_letters: .byte      "pdcaiwh"
CMDS        = * - cmd_letters
cmd_routines: .word     c_print, c_delete, c_change, c_add, c_insert, c_write, c_help
.assert     * - cmd_routines = CMDS * 2, error, "ed: a routine for each command"

s_lines:    .byte       " lines", 0
s_bytes:    .byte       " bytes", 0
s_new:      .byte       "new file", 0
s_noline:   .byte       "no such line", 0
s_which:    .byte       "which lines?", 0
s_full:     .byte       "full", 0
s_big:      .byte       "too big: only the start was read", 0
s_noname:   .byte       "no file name (w name)", 0
s_unsaved:  .byte       "not written: q again to quit anyway", 0
s_what:     .byte       "h: help", 0
s_help:     .byte       "p [a[,b]]  print (all)       a [n]      add after n (the last)", LF
            .byte       "i [n]      insert before n   c a[,b]    change", LF
            .byte       "d a[,b]    delete            w [file]   write", LF, 0
s_help2:    .byte       "q          quit              Q          quit, not writing", LF
            .byte       "n: a number, or $ (the last).  Lines typed after a, i or c end with a .", LF, 0
tl_name:    .byte       "ed", 0
tl_flagset: .byte       0
tl_usage:   .byte       "ed [file]", 0

.include "toollib.s"
