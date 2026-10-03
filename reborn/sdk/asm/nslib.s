; ****************************************************************************
; nslib.s - a task's default namespace, as Plan 9's newns makes it: included at the end of a program's source (as
; srvlib.s is), for init and the shells.  ns_default builds it in the calling task's own namespace (its children
; share it, as SPAWN gives it them):
;   1. its own area of the RAM disk, r/N (N: its task, in decimal), made, and emptied if a task before it with that
;      number left one there (#fr/N: the RAM disk's own name), with bin and lib in it (its caches)
;   2. the namespace file, /rom/lib/namespace (read from the ROM disk: #fx/lib/namespace), then a card's
;      (/sd/0/lib/namespace), if it has one: with $task as N, its own area is its /ram
; A namespace file: a line a command, its words separated by spaces or tabs; '...' quotes a word ('' in it: a ');
; # starts a comment at a word's start; $task in a word is the task's number.  The commands:
;   bind [-a | -b] [-c] new old             BIND: in place of what's at old, or after it (-a), or before it (-b);
;                                           -c: creates in the union go to new
;   mount [-a | -b] [-c] '#x' old [spec]    MOUNT of device x, with a spec
; A line that fails is said on stdout ("newns: the line: its error"), but for what isn't there (E_NOENT, E_NODEV,
; E_NOTFS: a card's bin, say), as Plan 9's newns is quiet about it; the rest go on.
;
; It uses: its zero page (ns_*), r0-r3, and the file calls.

NS_BUF_MAX      = 2048          ; A namespace file's bytes, at most (the rest aren't read)
NS_WORDS        = 6             ; A line's words, at most
NS_DEPTH        = 4             ; Directories deep an old area is emptied

.pushseg

.zeropage
ns_p:       .res        2               ; Where the line being read is in ns_buf ...
ns_end:     .res        2               ;   and the file's end
ns_w:       .res        2               ; Where the next word's byte goes (ns_wbuf)
ns_task:    .res        1               ; The task ($task)
ns_n:       .res        1               ; The line's words
ns_fl:      .res        1               ; A bind's or mount's flags
ns_line:    .res        2               ; The line's start (for its error)
ns_fd:      .res        1
ns_d:       .res        1               ; (ns_empty's depth)
ns_len:     .res        1               ;   and its path's length

.bss
ns_buf:     .res        NS_BUF_MAX + 1
ns_wbuf:    .res        128             ; The line's words, zero-terminated
ns_wptr:    .res        NS_WORDS * 2    ;   and where each one is
ns_path:    .res        64              ; An area's name, and the names under it
ns_rec:     .res        SR_SIZE
ns_msg:     .res        32

.code

; ****************************************************************************
; This task's default namespace: its RAM disk area, then the ROM's namespace file and a card's.  OUT: C = 0; or
; C = 1, .A = the error opening /rom/lib/namespace (none run: the caller's fallback)
ns_default:
            jsr         GETPID
            sta         ns_task
            jsr         ns_area
            LDR         r0, ns_s_rom
            jsr         ns_file
            bcs         @done
            LDR         r0, ns_s_card                       ; (A card's, if it has one: nothing, if not)
            jsr         ns_file
            clc
@done:
            rts

; Run the namespace file r0.  OUT: C = 0; or C = 1, .A = the error opening or reading it
ns_file:
            lda         #O_READ
            jsr         OPEN
            bcs         @done
            sta         ns_fd
            LDR         r0, ns_buf
            LDR         r1, NS_BUF_MAX
            lda         ns_fd
            jsr         READ                                ; (A file system's READ gives it all: up to 512 a
            php                                             ;   request, as many as it takes)
            pha
            phx
            lda         ns_fd
            jsr         CLOSE
            plx
            pla
            plp
            bcs         @done
            clc                                             ; ns_end: its end
            adc         #<ns_buf
            sta         ns_end
            txa
            adc         #>ns_buf
            sta         ns_end + 1
            lda         #0                                  ; (A 0 after it: a line's text ends there)
            sta         (ns_end)
            LDR         ns_p, ns_buf
@line:
            lda         ns_p                                ; The file's end: done
            cmp         ns_end
            lda         ns_p + 1
            sbc         ns_end + 1
            bcs         @end
            jsr         ns_words
            lda         ns_n
            beq         @line                               ; (Nothing on it)
            jsr         ns_command
            bcc         @line
            cmp         #E_NOENT                            ; (What isn't there: quiet)
            beq         @line
            cmp         #E_NODEV
            beq         @line
            cmp         #E_NOTFS
            beq         @line
            jsr         ns_said
            bra         @line

@end:
            clc
@done:
            rts

; The line at ns_p in words (ns_wptr, ns_n of them), ns_p past it.  Modifies: .A, .X, .Y
ns_words:
            MOVR        ns_line, ns_p
            LDR         ns_w, ns_wbuf
            stz         ns_n
@space:
            jsr         ns_peek
            beq         @eol
            cmp         #' '
            beq         @next
            cmp         #9                                  ; (Tab)
            beq         @next
            cmp         #'#'                                ; A comment: the rest of the line
            beq         @comment
            ldx         ns_n                                ; A word: where it starts
            cpx         #NS_WORDS
            bcs         @comment                            ; (Too many: the rest left out)
            txa
            asl
            tax
            lda         ns_w
            sta         ns_wptr,X
            lda         ns_w + 1
            sta         ns_wptr + 1,X
            inc         ns_n
            jsr         ns_word
            bra         @space

@next:
            jsr         ns_skip
            bra         @space

@comment:
            jsr         ns_peek
            beq         @eol
            jsr         ns_skip
            bra         @comment

@eol:                                                       ; The line's end: past its LF (or CR)
            lda         ns_p
            cmp         ns_end
            lda         ns_p + 1
            sbc         ns_end + 1
            bcs         :+
            jsr         ns_skip
:
            rts

; A word at ns_p into ns_wbuf (quotes off, $task its number), and a 0 after it
ns_word:
            jsr         ns_peek
            beq         @end
            cmp         #' '
            beq         @end
            cmp         #9
            beq         @end
            cmp         #$27                                ; A quote: what's in it, as it is
            beq         @quoted
            cmp         #'$'
            bne         @put
            ldy         #4                                  ; $task?
:
            lda         (ns_p),Y
            cmp         ns_s_task - 1,Y
            bne         :+
            dey
            bne         :-
            clc                                             ; (Its 5 bytes passed: the task's number)
            lda         ns_p
            adc         #5
            sta         ns_p
            bcc         @number
            inc         ns_p + 1
@number:
            jsr         ns_number
            bra         ns_word
:
            lda         #'$'
@put:
            jsr         ns_put
            jsr         ns_skip
            bra         ns_word

@quoted:
            jsr         ns_skip                             ; (The quote)
@in:
            jsr         ns_peek
            beq         @end                                ; (Unended: the line's end ends it)
            cmp         #$27
            beq         @close
            jsr         ns_put
            jsr         ns_skip
            bra         @in

@close:
            jsr         ns_skip
            jsr         ns_peek                             ; '' in it: a quote
            cmp         #$27
            bne         ns_word
            jsr         ns_put
            jsr         ns_skip
            bra         @in

@end:
            lda         #0
            jmp         ns_put

; .A = the byte at ns_p, Z = 1 at the line's end (an LF, a CR, a 0, or the file's).  Keeps .X, .Y
ns_peek:
            lda         ns_p
            cmp         ns_end
            lda         ns_p + 1
            sbc         ns_end + 1
            bcs         @eol
            lda         (ns_p)
            beq         @eol
            cmp         #LF
            beq         @eol
            cmp         #CR
            beq         @eol
            rts

@eol:
            lda         #0
            rts

; ns_p on a byte.  Keeps .A, .X, .Y
ns_skip:
            inc         ns_p
            bne         :+
            inc         ns_p + 1
:
            rts

; .A into ns_wbuf (127 bytes at most: the rest left out).  Keeps .X, .Y
ns_put:
            pha
            lda         ns_w
            cmp         #<(ns_wbuf + 127)
            lda         ns_w + 1
            sbc         #>(ns_wbuf + 127)
            pla
            bcs         :+
            sta         (ns_w)
            inc         ns_w
            bne         :+
            inc         ns_w + 1
:
            rts

; The task's number, in decimal, into ns_wbuf
ns_number:
            lda         ns_task
            cmp         #10
            bcc         :+
            sbc         #10                                 ; (16 tasks: 10-15)
            pha
            lda         #'1'
            jsr         ns_put
            pla
:
            ora         #'0'
            jmp         ns_put

; The line's command (ns_wptr: its words): its flags (the words after the first that start with -), then the
; rest.  OUT: C = 0; or C = 1, .A = the error
ns_command:
            stz         ns_fl
            ldx         #1                                  ; (.X: a word)
@flag:
            cpx         ns_n
            bcs         @args
            txa
            asl
            tay
            lda         ns_wptr,Y
            sta         r0
            lda         ns_wptr + 1,Y
            sta         r0 + 1
            lda         (r0)
            cmp         #'-'
            bne         @args
            ldy         #1
@letter:
            lda         (r0),Y
            beq         @flagged
            cmp         #'a'
            bne         :+
            lda         #MAFTER
            bra         @set
:
            cmp         #'b'
            bne         :+
            lda         #MBEFORE
            bra         @set
:
            cmp         #'c'
            bne         @inval
            lda         #MCREATE
@set:
            ora         ns_fl
            sta         ns_fl
            iny
            bra         @letter

@flagged:
            inx
            bra         @flag

@args:                                                      ; ns_d: the first word after them; ns_len: how many
            stx         ns_d                                ;   words from it
            sec
            lda         ns_n
            sbc         ns_d
            sta         ns_len
            LDR         r0, ns_s_bind
            jsr         ns_is
            beq         @bind
            LDR         r0, ns_s_mount
            jsr         ns_is
            beq         @mount
@inval:
            lda         #E_INVAL
            sec
            rts

@bind:                                                      ; bind new old
            lda         ns_len
            cmp         #2
            bne         @inval
            lda         ns_d
            asl
            tax
            jsr         ns_arg0
            jsr         ns_arg1
            lda         ns_fl
            jmp         BIND

@mount:                                                     ; mount '#x' old [spec]
            lda         ns_len
            cmp         #2
            beq         :+
            cmp         #3
            bne         @inval
:
            lda         ns_d
            asl
            tax
            jsr         ns_arg0                             ; (r0 = the device's name: #x)
            lda         (r0)
            cmp         #'#'
            bne         @inval
            ldy         #2
            lda         (r0),Y
            bne         @inval
            dey
            lda         (r0),Y                              ; (Its letter)
            pha
            jsr         ns_arg1                             ; r1 = old
            stz         r0                                  ; r0 = the spec, or 0
            stz         r0 + 1
            lda         ns_len
            cmp         #3
            bne         :+
            lda         ns_wptr + 4,X
            sta         r0
            lda         ns_wptr + 5,X
            sta         r0 + 1
:
            plx
            lda         ns_fl
            jmp         MOUNT

; r0 = word .X / 2's text; ns_arg1: r1 = the next one's.  Keeps .X
ns_arg0:
            lda         ns_wptr,X
            sta         r0
            lda         ns_wptr + 1,X
            sta         r0 + 1
            rts

ns_arg1:
            lda         ns_wptr + 2,X
            sta         r1
            lda         ns_wptr + 3,X
            sta         r1 + 1
            rts

; Is the line's first word the one at r0?  OUT: Z = 1: it is.  Modifies: .A, .Y, r1
ns_is:
            lda         ns_wptr
            sta         r1
            lda         ns_wptr + 1
            sta         r1 + 1
            ldy         #0
:
            lda         (r0),Y
            cmp         (r1),Y
            bne         @done
            iny
            cmp         #0
            bne         :-
@done:
            rts

; The line at ns_line, and its error .A, on stdout: "newns: the line: its error"
ns_said:
            pha
            LDR         r0, ns_s_newns
            jsr         PUTS
            ldy         #0
:
            lda         (ns_line),Y
            beq         :+
            cmp         #LF
            beq         :+
            cmp         #CR
            beq         :+
            phy
            jsr         PUTC
            ply
            iny
            bne         :-
:
            LDR         r0, ns_s_colon
            jsr         PUTS
            LDR         r0, ns_msg
            pla
            jsr         ERRSTR
            LDR         r0, ns_msg
            jsr         PUTS
            LDR         r0, ns_s_crlf
            jmp         PUTS

; ****************************************************************************
; This task's own area of the RAM disk: #fr/N made, emptied first if a task before it with that number left it.
; No RAM disk (or no room): no area, and nothing said (the namespace's /ram is then nothing)
ns_area:
            ldx         #0                                  ; ns_path = "#fr/N"
:
            lda         ns_s_area,X
            sta         ns_path,X
            beq         :+
            inx
            bra         :-
:
            stx         ns_len
            lda         ns_task
            cmp         #10
            bcc         :+
            sbc         #10
            pha
            lda         #'1'
            sta         ns_path,X
            inx
            pla
:
            ora         #'0'
            sta         ns_path,X
            inx
            stz         ns_path,X
            stx         ns_len
            stz         ns_d
            jsr         ns_empty                            ; (One left there: emptied)
            jsr         ns_mkdir                            ; Made (there already: as it is) ...
            ldx         #0                                  ;   and its bin and lib
            jsr         @in
            ldx         #ns_s_lib - ns_s_bin
@in:
            ldy         ns_len
            lda         #'/'
            sta         ns_path,Y
:
            iny
            lda         ns_s_bin,X
            sta         ns_path,Y
            inx
            cmp         #0
            bne         :-
            jsr         ns_mkdir
            ldx         ns_len
            stz         ns_path,X
            rts

; Make the directory ns_path (there already: as it is).  Modifies: .A, .X, .Y, r0
ns_mkdir:
            LDR         r0, ns_path
            lda         #O_READ
            ldx         #DM_DIR
            jsr         CREATE
            bcs         :+
            jmp         CLOSE
:
            rts

; Empty the directory ns_path (ns_len long; ns_d deep): each entry in turn (the first, read again each time), a
; file removed, a directory emptied then removed.  An error, or too deep: it stops
ns_empty:
@entry:
            LDR         r0, ns_path
            lda         #O_READ
            jsr         OPEN
            bcs         @done                               ; (Not there: nothing to empty)
            sta         ns_fd
            LDR         r0, ns_rec
            LDR         r1, SR_SIZE
            lda         ns_fd
            jsr         READ
            php
            pha
            lda         ns_fd
            jsr         CLOSE
            pla
            plp
            bcs         @done
            cmp         #SR_SIZE                            ; (None left: emptied)
            bne         @done
            ldx         ns_len                              ; ns_path: then / and its name
            lda         #'/'
            sta         ns_path,X
            inx
            ldy         #0
:
            lda         ns_rec + SR_NAME,Y
            sta         ns_path,X
            beq         :+
            inx
            iny
            cpx         #63
            bcc         :-
            bra         @back                               ; (Too long a name)
:
            lda         ns_rec + SR_QTYPE
            bpl         @remove
            lda         ns_d                                ; A directory: emptied first
            cmp         #NS_DEPTH
            bcs         @back
            inc         ns_d
            lda         ns_len
            pha
            stx         ns_len
            jsr         ns_empty
            pla
            sta         ns_len
            dec         ns_d
@remove:
            LDR         r0, ns_path
            jsr         REMOVE
            php
            ldx         ns_len                              ; (ns_path as it was)
            stz         ns_path,X
            plp
            bcc         @entry
            rts

@back:
            ldx         ns_len
            stz         ns_path,X
@done:
            rts

.rodata
ns_s_rom:   .byte       "#fx/lib/namespace", 0
ns_s_card:  .byte       "/sd/0/lib/namespace", 0
ns_s_area:  .byte       "#fr/", 0
ns_s_task:  .byte       "task"
ns_s_bin:   .byte       "bin", 0
ns_s_lib:   .byte       "lib", 0
ns_s_bind:  .byte       "bind", 0
ns_s_mount: .byte       "mount", 0
ns_s_newns: .byte       "newns: ", 0
ns_s_colon: .byte       ": ", 0
ns_s_crlf:  .byte       CR, LF, 0

.popseg
