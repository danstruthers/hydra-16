; ****************************************************************************
; builtin.s - rc's built-ins: what changes rc's own task (its directory, namespace, variables, input), and so
; can't be a program.
;   cd [dir]                    CHDIR (no dir: $home, else /)
;   exit [status]               rc ends ($status: the words given)
;   wait [task]                 the background tasks waited for (or one)
;   eval words                  the words as a command line
;   . file [args]               the file's commands, here ($* its args meanwhile)
;   builtin cmd args            cmd as a built-in or a program (not a function)
;   whatis names                each as rc sees it: a variable, a function, a built-in, or the program run
;   shift [n]                   $*'s first n (1) words gone
;   bind [-abc] new old         BIND (-a after, -b before; -c: creates there)
;   mount [-abc] '#x' old [spec] MOUNT of device x
;   unmount [new] old           UNMOUNT
;   newns                       the default namespace (nslib.s: Plan 9's newns)

.include "rc.inc"

.zeropage
bw:         .res        2                                   ; A word of the arguments

.bss
bi:         .res        1                                   ; The built-in found
bfl:        .res        1                                   ; bind's and mount's flags
bq:         .res        1                                   ; <> 0: put_word's word quoted
bbuf:       .res        PATH_MAX + 1                        ; A word, zero-terminated
bbuf2:      .res        PATH_MAX + 1

.code

; The built-in named .A bytes at p1.  OUT: C = 0 (bi: which); or C = 1: none
builtin_find:
            sta         t0
            ldx         #0                                  ; (.X: in the names; bi: which)
            stz         bi
@name:
            lda         b_names,X
            beq         @none
            ldy         #0
:
            lda         b_names,X
            beq         @end
            cpy         t0
            beq         @skip
            cmp         (p1),Y
            bne         @skip
            inx
            iny
            bra         :-

@end:
            cpy         t0
            bne         @skip
            clc
            rts

@skip:
            lda         b_names,X                           ; To the next name
            beq         :+
            inx
            bra         @skip
:
            inx
            inc         bi
            bra         @name

@none:
            sec
            rts

; Built-in bi run: its words xw (the name first).  Its status set
builtin_run:
            lda         xw
            ldx         xw + 1
            jsr         list_next                           ; bw: the first argument
            sta         bw
            stx         bw + 1
            lda         bi
            asl
            tax
            jmp         (b_vec,X)

; ---- cd [dir]
b_cd:
            lda         (bw)
            cmp         #LIST_END
            bne         @dir
            LDR         p1, s_home                          ; $home, or /
            lda         #4
            jsr         var_get
            sta         bw
            stx         bw + 1
            lda         (bw)
            cmp         #LIST_END
            bne         @dir
            LDR         r0, s_root
            bra         @go

@dir:
            jsr         word_c                              ; (bbuf)
            LDR         r0, bbuf
@go:
            jsr         CHDIR
            bcc         ok
            jmp         failed_word

; $status true (none)
ok:
            lda         #0
            jmp         status_word

; ---- exit [status]
b_exit:
            lda         (bw)
            cmp         #LIST_END
            beq         :+
            MOVR        p2, bw
            LDR         p1, s_status
            lda         #6
            jsr         var_set
:
            jmp         rc_exits

; ---- wait [task]
b_wait:
            lda         #$FF
            sta         bfl
            lda         (bw)
            cmp         #LIST_END
            beq         :+
            jsr         word_num
            sta         bfl
:
            lda         bfl
            jmp         wait_bg

; ---- eval words
b_eval:
            lda         bw
            ldx         bw + 1
            jsr         list_flat
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
            ldx         #0
            jmp         run_text

; ---- . file [args]
b_dot:
            lda         (bw)
            cmp         #LIST_END
            bne         :+
            LDR         r0, s_dotuse
            jmp         rc_error
:
            jsr         word_c
            LDR         r0, bbuf
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         failed_word
:
            pha                                             ; (The fd)
            jsr         var_local_mark
            pha
            LDR         p1, s_star                          ; $*: its args
            lda         #1
            jsr         var_local_save
            lda         bw
            ldx         bw + 1
            jsr         list_next
            sta         p2
            stx         p2 + 1
            jsr         var_local_set
            pla
            plx                                             ; (The fd: .X)
            pha
            txa
            jsr         run_file                            ; (rc.s: its commands, to its end; the fd closed)
            pla
            jmp         var_local_restore

; ---- builtin cmd args
b_builtin:
            lda         (bw)
            cmp         #LIST_END
            beq         @done
            MOVR        xw, bw
            clc
            lda         xw
            adc         #1
            sta         p1
            lda         xw + 1
            adc         #0
            sta         p1 + 1
            lda         (xw)
            jsr         builtin_find
            bcs         :+
            jmp         builtin_run
:
            lda         xw
            ldx         xw + 1
            stz         xf
            jsr         spawn_path
            bcs         :+
            jmp         wait_child
:
            jmp         status_error

@done:
            jmp         ok

; ---- whatis names
b_whatis:
            lda         (bw)
            cmp         #LIST_END
            bne         :+
            jmp         ok
:
            clc
            lda         bw
            adc         #1
            sta         p1
            lda         bw + 1
            adc         #0
            sta         p1 + 1
            lda         (bw)
            pha
            jsr         var_find                            ; A variable: name=(its words)
            pla
            pha
            bcs         @fn
            ldy         #3                                  ; (Set: a value)
            lda         (vr),Y
            iny
            ora         (vr),Y
            beq         @fn
            jsr         put_name
            lda         #'='
            jsr         PUTC
            ldy         #3                                  ; One word: as it is; else (its words)
            lda         (vr),Y
            sta         p3
            iny
            lda         (vr),Y
            sta         p3 + 1
            lda         p3
            ldx         p3 + 1
            jsr         list_count
            cmp         #1
            bne         :+
            jsr         put_word
            bra         :++
:
            lda         #'('
            jsr         PUTC
            lda         p3
            ldx         p3 + 1
            jsr         put_list
            lda         #')'
            jsr         PUTC
:
            jsr         put_nl
@fn:
            pla
            pha
            jsr         var_fn                              ; A function: fn name {text}
            pla
            pha
            bcs         @builtin
            LDR         r0, s_fn
            jsr         PUTS
            jsr         put_name
            LDR         r0, s_brace
            jsr         PUTS
            ldy         #3
            lda         (vr),Y
            sta         p0
            iny
            lda         (vr),Y
            sta         p0 + 1
            ldy         #1
            lda         (p0),Y
            sta         num + 1
            lda         (p0)
            sta         num
            clc
            lda         p0
            adc         #2
            sta         p0
            bcc         :+
            inc         p0 + 1
:
@ftext:
            lda         num
            ora         num + 1
            beq         @fend
            lda         (p0)
            jsr         PUTC
            inc         p0
            bne         :+
            inc         p0 + 1
:
            lda         num
            bne         :+
            dec         num + 1
:
            dec         num
            bra         @ftext

@fend:
            lda         #'}'
            jsr         PUTC
            jsr         put_nl
            bra         @next

@builtin:
            jsr         builtin_find
            bcs         @program
            LDR         r0, s_builtin
            jsr         PUTS
            jsr         put_name
            jsr         put_nl
            bra         @next

@program:                                                   ; A program: its path, as $path finds it
            jsr         find_path
            bcs         @next
            LDR         r0, pathbuf_b
            jsr         PUTS
            jsr         put_nl
@next:
            pla
            lda         bw
            ldx         bw + 1
            jsr         list_next
            sta         bw
            stx         bw + 1
            jmp         b_whatis

; The word bw's text (p1, the length on the stack's top as whatis has it)
put_name:
            lda         (bw)
            tax
            beq         @done
            ldy         #1
:
            lda         (bw),Y
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

; The list .A/.X's words, with spaces between them, each as put_word has it
put_list:
            sta         p3
            stx         p3 + 1
            ldx         #0
@word:
            lda         (p3)
            cmp         #LIST_END
            beq         @done
            cpx         #0
            beq         :+
            lda         #' '
            jsr         PUTC
:
            jsr         put_word
            lda         p3
            ldx         p3 + 1
            jsr         list_next
            sta         p3
            stx         p3 + 1
            ldx         #1
            bra         @word

@done:
            rts

; The word at p3 as rc reads it: quoted ('...', a ' in it doubled) if it's empty or has a space, a control
; character or one of `^#*[]=|\?${}()'<>&; (Plan 9's needsrcquote)
put_word:
            lda         (p3)
            sta         bq                                  ; (Empty: quoted)
            beq         @quote
            tay
@look:
            lda         (p3),Y
            cmp         #' ' + 1
            bcc         @quote
            ldx         #s_special_end - s_special - 1
:
            cmp         s_special,X
            beq         @quote
            dex
            bpl         :-
            dey
            bne         @look
            stz         bq                                  ; (None: as it is)
            bra         @put

@quote:
            lda         #$27
            sta         bq
            jsr         PUTC
@put:
            lda         (p3)
            tax
            beq         @end
            ldy         #1
:
            lda         (p3),Y
            phy
            phx
            pha
            jsr         PUTC
            pla
            cmp         #$27                                ; (A ' in a quoted word: '')
            bne         :+
            lda         bq
            beq         :+
            jsr         PUTC
:
            plx
            ply
            iny
            dex
            bne         :--
@end:
            lda         bq
            beq         :+
            jsr         PUTC
:
            rts

; A new line (PUTC)
put_nl:
            lda         #LF
            jmp         PUTC

; The program named by word bw found in $path (STAT of each dir/name), into pathbuf_b.  OUT: C = 0; or C = 1
find_path:
            LDR         p1, s_path
            lda         #4
            jsr         var_get
            sta         p3
            stx         p3 + 1
@dir:
            lda         (p3)
            cmp         #LIST_END
            beq         @none
            ldx         #0
            tay
            beq         :++
            ldy         #1
:
            lda         (p3),Y
            sta         pathbuf_b,X
            inx
            iny
            tya
            dec         a
            cmp         (p3)
            bne         :-
:
            lda         #'/'
            sta         pathbuf_b,X
            inx
            lda         (bw)
            tay
            beq         @try
            ldy         #1
:
            lda         (bw),Y
            sta         pathbuf_b,X
            inx
            cpx         #PATH_MAX
            bcs         @next
            iny
            tya
            dec         a
            cmp         (bw)
            bne         :-
@try:
            stz         pathbuf_b,X
            LDR         r0, pathbuf_b
            LDR         r1, bstat
            jsr         STAT
            bcc         @found
@next:
            lda         p3
            ldx         p3 + 1
            jsr         list_next
            sta         p3
            stx         p3 + 1
            bra         @dir

@none:
            sec
@found:
            rts

; ---- shift [n]
b_shift:
            lda         #1
            sta         bfl
            lda         (bw)
            cmp         #LIST_END
            beq         :+
            jsr         word_num
            sta         bfl
:
            LDR         p1, s_star
            lda         #1
            jsr         var_get
            sta         p2
            stx         p2 + 1
@drop:
            lda         bfl
            beq         @set
            lda         (p2)
            cmp         #LIST_END
            beq         @set
            lda         p2
            ldx         p2 + 1
            jsr         list_next
            sta         p2
            stx         p2 + 1
            dec         bfl
            bra         @drop

@set:
            lda         p2                                  ; (A copy first: var_set frees the old value)
            ldx         p2 + 1
            jsr         list_dup
            sta         p2
            stx         p2 + 1
            LDR         p1, s_star
            lda         #1
            jsr         var_set
            jmp         ok

; ---- bind [-abc] new old
b_bind:
            jsr         flags
            jsr         need2
            jsr         word_c                              ; new, old
            jsr         next_word
            jsr         word_c2
            LDR         r0, bbuf
            LDR         r1, bbuf2
            lda         bfl
            jsr         BIND
            bcc         :+
            jmp         failed_word
:
            jmp         ok

; ---- mount [-abc] '#x' old [spec]
b_mount:
            jsr         flags
            jsr         need2
            ldy         #1                                  ; '#x': x
            lda         (bw),Y
            cmp         #'#'
            bne         @usage
            iny
            lda         (bw),Y
            pha
            jsr         next_word
            jsr         word_c2                             ; old
            jsr         next_word
            stz         r0
            stz         r0 + 1
            lda         (bw)
            cmp         #LIST_END
            beq         :+
            jsr         word_c                              ; spec
            LDR         r0, bbuf
:
            LDR         r1, bbuf2
            plx
            lda         bfl
            jsr         MOUNT
            bcc         :+
            jmp         failed_word
:
            jmp         ok

@usage:
            LDR         r0, s_mountuse
            jmp         rc_error

; ---- unmount [new] old
b_unmount:
            lda         (bw)
            cmp         #LIST_END
            bne         :+
            LDR         r0, s_unmountuse
            jmp         rc_error
:
            lda         bw                                  ; One word: old; two: new old
            ldx         bw + 1
            jsr         list_next
            sta         p0
            stx         p0 + 1
            lda         (p0)
            cmp         #LIST_END
            beq         @one
            jsr         word_c
            jsr         next_word
            jsr         word_c2
            LDR         r0, bbuf
            bra         @go

@one:
            jsr         word_c2
            stz         r0
            stz         r0 + 1
@go:
            LDR         r1, bbuf2
            jsr         UNMOUNT
            bcc         :+
            jmp         failed_word
:
            jmp         ok

; ---- newns
b_newns:
            jsr         ns_default
            jmp         ok

; ---- The pieces

; -a -b -c at bw's start: bfl (MREPL, MBEFORE, MAFTER, | MCREATE); bw past them
flags:
            stz         bfl
@word:
            lda         (bw)
            cmp         #LIST_END
            beq         @done
            ldy         #1
            lda         (bw),Y
            cmp         #'-'
            bne         @done
            lda         (bw)
            tax
@char:
            iny
            dex
            beq         @next
            lda         (bw),Y
            cmp         #'a'
            bne         :+
            lda         #MAFTER
            tsb         bfl
:
            cmp         #'b'
            bne         :+
            lda         #MBEFORE
            tsb         bfl
:
            cmp         #'c'
            bne         :+
            lda         #MCREATE
            tsb         bfl
:
            bra         @char

@next:
            jsr         next_word
            bra         @word

@done:
            rts

; Two words at least at bw (else: a usage error)
need2:
            lda         (bw)
            cmp         #LIST_END
            beq         @usage
            lda         bw
            ldx         bw + 1
            jsr         list_next
            sta         p0
            stx         p0 + 1
            lda         (p0)
            cmp         #LIST_END
            beq         @usage
            rts

@usage:
            LDR         r0, s_twowords
            jmp         rc_error

; bw on to its next word
next_word:
            lda         bw
            ldx         bw + 1
            jsr         list_next
            sta         bw
            stx         bw + 1
            rts

; Word bw into bbuf (or bbuf2: word_c2), zero-terminated (PATH_MAX at most)
word_c:
            LDR         p0, bbuf
            bra         wc

word_c2:
            LDR         p0, bbuf2
wc:
            lda         (bw)
            cmp         #PATH_MAX + 1
            bcc         :+
            lda         #PATH_MAX
:
            tax
            tay
            lda         #0
            sta         (p0),Y
            cpx         #0
            beq         @done
:
            lda         (bw),Y
            dey
            sta         (p0),Y
            bne         :-
@done:
            rts

; Word bw as a number (decimal): .A
word_num:
            jsr         word_c
            stz         t0
            ldx         #0
:
            lda         bbuf,X
            beq         :+
            sec
            sbc         #'0'
            cmp         #10
            bcs         :+
            pha
            lda         t0
            asl
            asl
            adc         t0
            asl
            sta         t0
            pla
            clc
            adc         t0
            sta         t0
            inx
            bra         :-
:
            lda         t0
            rts

; A built-in's call failed (.A): "rc: name: its text", $status it
failed_word:
            jmp         status_error

.rodata
b_names:    .byte       "cd", 0, "exit", 0, "wait", 0, "eval", 0, ".", 0, "builtin", 0, "whatis", 0, "shift", 0
            .byte       "bind", 0, "mount", 0, "unmount", 0, "newns", 0, 0
b_vec:      .word       b_cd, b_exit, b_wait, b_eval, b_dot, b_builtin, b_whatis, b_shift, b_bind, b_mount
            .word       b_unmount, b_newns
s_home:     .byte       "home"
s_root:     .byte       "/", 0
; (put_word's: a word with one of these is quoted)
s_special:  .byte       "`^#*[]=|", $5C, "?${}()", $27, "<>&;"
s_special_end:
s_star:     .byte       "*"
s_status:   .byte       "status"
s_path:     .byte       "path"
s_fn:       .byte       "fn ", 0
s_brace:    .byte       " {", 0
s_builtin:  .byte       "builtin ", 0
s_dotuse:   .byte       "usage: . file [args]", 0
s_mountuse: .byte       "usage: mount [-abc] '#x' old [spec]", 0
s_unmountuse: .byte     "usage: unmount [new] old", 0
s_twowords: .byte       "usage: bind [-abc] new old", 0

.bss
pathbuf_b:  .res        PATH_MAX + 1
bstat:      .res        SR_SIZE

.include "nslib.s"
