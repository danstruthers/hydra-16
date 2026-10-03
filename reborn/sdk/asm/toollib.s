; ****************************************************************************
; toollib.s - what the tools have in common: included at the end of a tool's source (as nslib.s is).  The tool
; defines tl_name (its name, zero-terminated: the first word of what it says on fd 2), tl_flagset (the flags it
; takes, a letter each, zero-terminated) and tl_usage (how it's used, after "usage: ", zero-terminated).
;   tl_start    main's arguments (r0): the flags (-abc: tl_flags bit 0 for tl_flagset's first letter, bit 1 for
;               its second ...; - alone or a word not starting with - ends them, and -- is their end), tl_arg
;               the first argument after them.  A letter it doesn't take: the usage said on fd 2, and the tool
;               ends ("usage").  OUT: .A = tl_flags
;   tl_next     tl_arg on to the next argument.  OUT: .A = its first byte (0, Z = 1: there's none)
;   tl_count    .A = the arguments from tl_arg on (255 at most)
;   tl_putc     .A into the output (fd 1, written 256 bytes at a time); tl_puts the string at r0, tl_space,
;               tl_nl; tl_dec tl_num (4 bytes: changed) in decimal, .A characters at least (spaces first).
;               Each keeps .X, .Y, r0 and r1
;   tl_flush    the output written.  A write that fails: "NAME: write error: why" on fd 2, and the tool ends
;               ("write error"); one a note ended (a tool with a note handler: edit) drops what was left
;   tl_err      error .A about the name at r0: "NAME: name: why" on fd 2 (the output written first); tl_code 1
;   tl_warn     the message at r0: "NAME: message" on fd 2 (the output written first); tl_code 1
;   tl_end      the output written, and EXITS with tl_code
;   tl_pcat     with tl_pp at a path (PATH_MAX + 1 bytes): a / (unless it's empty or ends with one) and the string
;               at r0 added to it.  OUT: C = 0, .A = its old length (for tl_pcut); or C = 1, .A = E_NAMETOOLONG
;               (as it was)
;   tl_pcut     the path at tl_pp cut to .A bytes
;   tl_base     r0 = the last part of the path at r0 (past its last /)
;   tl_readdir  the directory at r0 read whole, into memory past the break (BREAK).  OUT: C = 0, r0 = its
;               records (SR_SIZE each), .A/.X = their count; or C = 1, .A = the error.  tl_free (r0: those
;               records) gives the memory back; directories read one inside another are given back in turn
;   tl_walk     the tree in the directory at tl_pp, depth first, a routine for each entry (below)
;   tl_eachin   each input named from tl_arg on (none: fd 0) given to the routine at tl_ivec, open for tl_getc (the
;               next byte: .A, C = 1 at its end) and its name at tl_iname; tl_inopen, tl_inclose for one alone
;   tl_atoi     tl_num = the decimal number at r0 (C = 1: not one); tl_setnum: tl_num = .A
;   tl_div      tl_num = tl_num / tl_den (4 bytes each), tl_rem the remainder
;   tl_date     the time at r0 (4 bytes: seconds since 2000) into the output: "2000-01-31 12:59"
;   tl_field    the string at r0 into the output, then spaces to .A characters (one at least)
;   tl_badusage the usage said on fd 2, and the tool ends ("usage")
; It uses: its zero page (tl_*: toollib.inc, included at the tool's top), r0-r3, and the file calls.

.pushseg

.bss
tl_flags:   .res        1                                   ; The flags given (tl_start)
tl_code:    .res        1                                   ; The exit code: 1 after an error
tl_out:     .res        256                                 ; The output, waiting to be written ...
tl_olen:    .res        1                                   ;   its bytes
tl_msg:     .res        32                                  ; An error's text
tl_num:     .res        4                                   ; A number (tl_dec, tl_div)
tl_den:     .res        4                                   ; (tl_div's divisor ...
tl_rem:     .res        4                                   ;   and remainder)
tl_t:       .res        4                                   ; (Scratch)
tl_digits:  .res        10                                  ; (tl_dec's)
tl_width:   .res        1
tl_plen:    .res        1                                   ; (tl_pcat's: the old length)
tl_fd:      .res        1                                   ; (tl_readdir's: the directory ...
tl_dbase:   .res        2                                   ;   where its records start ...
tl_dtop:    .res        2                                   ;   and end)
tl_year:    .res        2                                   ; (tl_date's)
tl_month:   .res        1
tl_vdir:    .res        2                                   ; tl_walk's routines: a directory's, before ...
tl_vpost:   .res        2                                   ;   and after what's in it ...
tl_vfile:   .res        2                                   ;   and anything else's
tl_depth:   .res        1                                   ; tl_walk's directories open, and each one's ...
tl_wbasel:  .res        TL_DEPTH                            ;   records ...
tl_wbaseh:  .res        TL_DEPTH
tl_wrecl:   .res        TL_DEPTH                            ;   the next one ...
tl_wrech:   .res        TL_DEPTH
tl_wleftl:  .res        TL_DEPTH                            ;   how many are left ...
tl_wlefth:  .res        TL_DEPTH
tl_wplen:   .res        TL_DEPTH                            ;   and its path's length
tl_ifd:     .res        1                                   ; The input (tl_getc): its fd ($FF: fd 0) ...
tl_ibuf:    .res        256                                 ;   what's been read of it ...
tl_ilen:    .res        1                                   ;   how much ...
tl_ipos:    .res        1                                   ;   and how much of that's been taken
tl_iname:   .res        2                                   ; Its name, for its errors
tl_ivec:    .res        2                                   ; tl_eachin's routine

.code

; ****************************************************************************
; Arguments

; The flags, then tl_arg at the first argument after them.  OUT: .A = tl_flags
tl_start:
            MOVR        tl_arg, r0
            stz         tl_flags
            stz         tl_code
            stz         tl_olen
@arg:
            lda         (tl_arg)                            ; -letters?
            cmp         #'-'
            bne         @done
            ldy         #1
            lda         (tl_arg),Y
            beq         @done                               ; (- alone: an argument)
            cmp         #'-'
            bne         @letter
            iny
            lda         (tl_arg),Y
            bne         tl_badusage                         ; (--x)
            jsr         tl_next                             ; (--: the flags' end)
            bra         @done

@letter:
            lda         (tl_arg),Y
            beq         @next
            ldx         #0
:
            lda         tl_flagset,X                        ; One it takes?
            beq         tl_badusage
            cmp         (tl_arg),Y
            beq         :+
            inx
            bra         :-
:
            lda         tl_bits,X
            tsb         tl_flags
            iny
            bra         @letter

@next:
            jsr         tl_next
            bra         @arg

@done:
            lda         tl_flags
            rts

; The usage said on fd 2 ("usage: ..."), and the tool ends ("usage")
tl_badusage:
            jsr         tl_flush
            LDR         r0, tl_s_usage
            jsr         tl_puts2
            LDR         r0, tl_usage
            jsr         tl_puts2
            LDR         r0, tl_s_nl
            jsr         tl_puts2
            LDR         r0, tl_s_ustat
            lda         #1
            jmp         EXITS

; tl_arg on to the next argument.  OUT: .A = its first byte (0, Z = 1: there's none)
tl_next:
            lda         (tl_arg)
            beq         @done                               ; (None: it stays)
@skip:
            inc         tl_arg
            bne         :+
            inc         tl_arg + 1
:
            lda         (tl_arg)
            bne         @skip
            inc         tl_arg                              ; (Past its 0)
            bne         :+
            inc         tl_arg + 1
:
            lda         (tl_arg)
@done:
            rts

; .A = the arguments from tl_arg on (255 at most)
tl_count:
            lda         tl_arg + 1
            pha
            lda         tl_arg
            pha
            ldx         #0
:
            lda         (tl_arg)
            beq         :+
            inx
            jsr         tl_next
            cpx         #255
            bne         :-
:
            pla
            sta         tl_arg
            pla
            sta         tl_arg + 1
            txa
            rts

; ****************************************************************************
; The output

; .A into the output (full: written).  Keeps .X, .Y, r0, r1
tl_putc:
            phx
            ldx         tl_olen
            sta         tl_out,X
            inx
            stx         tl_olen
            bne         :+
            lda         #0                                  ; (Full: 256)
            jsr         tl_write
:
            plx
            rts

; The string at r0 into the output.  Keeps .X, .Y, r0, r1
tl_puts:
            phy
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            jsr         tl_putc
            iny
            bne         :-
:
            ply
            rts

; A space, or a new line, into the output
tl_space:
            lda         #' '
            bra         tl_putc

tl_nl:
            lda         #LF
            bra         tl_putc

; The output written
tl_flush:
            lda         tl_olen
            bne         tl_write
            rts

; The same, .A bytes of it (0: 256).  Keeps .X, .Y, r0, r1
tl_write:
            phx
            phy
            tax                                             ; (The count: .X)
            lda         r1 + 1                              ; (r0, r1 kept)
            pha
            lda         r1
            pha
            lda         r0 + 1
            pha
            lda         r0
            pha
            stx         r1
            stz         r1 + 1
            txa
            bne         :+
            inc         r1 + 1                              ; (0: 256)
:
            LDR         r0, tl_out
            lda         #1
            jsr         WRITE
            bcc         :+
            cmp         #E_INTR                             ; (A note ended it: a tool with a note handler
            bne         @failed                             ;   goes on, what was left dropped)
:
            stz         tl_olen
            pla
            sta         r0
            pla
            sta         r0 + 1
            pla
            sta         r1
            pla
            sta         r1 + 1
            ply
            plx
            rts

@failed:                                                    ; It can't be written: said, and the tool ends
            stz         tl_olen
            pha
            LDR         r0, tl_s_werr
            jsr         tl_prefix
            LDR         r0, tl_s_colon
            jsr         tl_puts2
            pla
            jsr         tl_why
            LDR         r0, tl_s_werr
            lda         #1
            jmp         EXITS

; tl_num in decimal, .A characters at least (spaces before it).  Changes tl_num.  Keeps .X, .Y, r0, r1
tl_dec:
            phx
            phy
            sta         tl_width
            ldy         #0                                  ; (.Y: its digits)
            ldx         #0                                  ; (.X: the power of 10, 10^9 first)
@power:
            lda         #'0'
            sta         tl_t + 3
@sub:
            sec                                             ; tl_num - the power, if it's no less
            lda         tl_num
            sbc         tl_p10_0,X
            sta         tl_t
            lda         tl_num + 1
            sbc         tl_p10_1,X
            sta         tl_t + 1
            lda         tl_num + 2
            sbc         tl_p10_2,X
            sta         tl_t + 2
            lda         tl_num + 3
            sbc         tl_p10_3,X
            bcc         @digit
            sta         tl_num + 3
            lda         tl_t + 2
            sta         tl_num + 2
            lda         tl_t + 1
            sta         tl_num + 1
            lda         tl_t
            sta         tl_num
            inc         tl_t + 3
            bra         @sub

@digit:
            lda         tl_t + 3
            cpy         #0                                  ; (A 0 before any other digit: none, but the last)
            bne         :+
            cmp         #'0'
            bne         :+
            cpx         #9
            bne         @on
:
            sta         tl_digits,Y
            iny
@on:
            inx
            cpx         #10
            bne         @power
            sty         tl_t                                ; Spaces first, to its width
@pad:
            lda         tl_width
            cmp         tl_t
            bcc         :+
            beq         :+
            jsr         tl_space
            dec         tl_width
            bra         @pad
:
            ldy         #0
:
            lda         tl_digits,Y
            jsr         tl_putc
            iny
            cpy         tl_t
            bne         :-
            ply
            plx
            rts

; tl_num = .A
tl_setnum:
            sta         tl_num
            stz         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            rts

; The string at r0 into the output, then spaces to .A characters (one at least).  Keeps r0
tl_field:
            sta         tl_t
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            jsr         tl_putc
            iny
            bne         :-
:
            jsr         tl_space
            iny
            cpy         tl_t
            bcc         :-
            rts

; ****************************************************************************
; Errors

; Error .A about the name at r0: "NAME: name: why" on fd 2; tl_code 1
tl_err:
            pha
            jsr         tl_prefix
            LDR         r0, tl_s_colon
            jsr         tl_puts2
            pla
            jsr         tl_why
            lda         #1
            sta         tl_code
            rts

; The message at r0: "NAME: message" on fd 2; tl_code 1
tl_warn:
            jsr         tl_prefix
            LDR         r0, tl_s_nl
            jsr         tl_puts2
            lda         #1
            sta         tl_code
            rts

; The output written, then "NAME: " and the string at r0 on fd 2
tl_prefix:
            jsr         tl_flush
            MOVR        tl_q, r0
            LDR         r0, tl_name
            jsr         tl_puts2
            LDR         r0, tl_s_colon
            jsr         tl_puts2
            MOVR        r0, tl_q
; The string at r0 on fd 2
tl_puts2:
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            iny
            bne         :-
:
            sty         r1
            stz         r1 + 1
            lda         #2
            jmp         WRITE

; Error .A's text and a new line on fd 2
tl_why:
            pha
            LDR         r0, tl_msg
            pla
            jsr         ERRSTR
            LDR         r0, tl_msg
            jsr         tl_puts2
            LDR         r0, tl_s_nl
            jmp         tl_puts2

; The output written, and EXITS with tl_code
tl_end:
            jsr         tl_flush
            stz         r0
            stz         r0 + 1
            lda         tl_code
            jmp         EXITS

; ****************************************************************************
; Paths

; A / (unless the path's empty or ends with one) and the string at r0, added to the path at tl_pp.  OUT: C = 0,
; .A = its old length; or C = 1, .A = E_NAMETOOLONG (as it was)
tl_pcat:
            MOVR        tl_q, r0
            ldy         #$FF
:
            iny
            lda         (tl_pp),Y
            bne         :-
            sty         tl_plen
            tya
            beq         @name                               ; (Empty: no /)
            dey
            lda         (tl_pp),Y
            iny
            cmp         #'/'
            beq         @name
            lda         #'/'
            sta         (tl_pp),Y
            iny
@name:
            lda         (tl_q)
            beq         @end
            cpy         #PATH_MAX
            bcs         @long
            sta         (tl_pp),Y
            iny
            inc         tl_q
            bne         @name
            inc         tl_q + 1
            bra         @name

@end:
            sta         (tl_pp),Y
            lda         tl_plen
            clc
            rts

@long:
            lda         tl_plen
            jsr         tl_pcut
            lda         #E_NAMETOOLONG
            sec
            rts

; The path at tl_pp cut to .A bytes
tl_pcut:
            tay
            lda         #0
            sta         (tl_pp),Y
            rts

; r0 = the last part of the path at r0 (past its last /)
tl_base:
            ldy         #0
            ldx         #0                                  ; (.X: past the last / so far)
:
            lda         (r0),Y
            beq         :++
            iny
            cmp         #'/'
            bne         :+
            tya
            tax
:
            bra         :--
:
            txa
            clc
            adc         r0
            sta         r0
            bcc         :+
            inc         r0 + 1
:
            rts

; ****************************************************************************
; Directories

; The directory at r0 read whole, past the break.  OUT: C = 0, r0 = its records, .A/.X = their count; or C = 1,
; .A = the error
tl_readdir:
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            rts

:
            sta         tl_fd
            stz         r0                                  ; Where they go: the break
            stz         r0 + 1
            jsr         BREAK
            MOVR        tl_dbase, r0
            MOVR        tl_dtop, r0
@read:
            clc                                             ; Room for 512 more
            lda         tl_dtop
            sta         r0
            lda         tl_dtop + 1
            adc         #>512
            sta         r0 + 1
            jsr         BREAK
            bcs         @failed
            MOVR        r0, tl_dtop
            LDR         r1, 512
            lda         tl_fd
            jsr         READ
            bcs         @failed
            sta         tl_t
            stx         tl_t + 1
            ora         tl_t + 1
            beq         @end
            clc
            lda         tl_dtop
            adc         tl_t
            sta         tl_dtop
            lda         tl_dtop + 1
            adc         tl_t + 1
            sta         tl_dtop + 1
            bra         @read

@end:
            lda         tl_fd
            jsr         CLOSE
            MOVR        r0, tl_dtop                         ; (The break: past them)
            jsr         BREAK
            MOVR        r0, tl_dbase
            sec                                             ; Their count: their bytes / 64
            lda         tl_dtop
            sbc         tl_dbase
            sta         tl_t
            lda         tl_dtop + 1
            sbc         tl_dbase + 1
            ldx         #6
:
            lsr         a
            ror         tl_t
            dex
            bne         :-
            tax
            lda         tl_t
            clc
            rts

@failed:
            pha
            lda         tl_fd
            jsr         CLOSE
            MOVR        r0, tl_dbase
            jsr         BREAK
            pla
            sec
            rts

; The memory of the records at r0 (tl_readdir's) given back
tl_free:
            jmp         BREAK

; The tree in the directory at tl_pp, depth first.  For each entry, with tl_pp its path and r0 its stat record:
; a directory's (tl_vdir: C = 1 to skip what's in it), the tree in it, then (tl_vpost); anything else's (tl_vfile).
; The routines may change r0-r3, .A, .X, .Y (not tl_pp's path).  A directory that can't be read, or is deeper than
; TL_DEPTH, is said (tl_err) and skipped; tl_depth is the directories open (1 in the first)
tl_walk:
            ldx         tl_depth
            cpx         #TL_DEPTH
            bcc         :+
            lda         #E_NAMETOOLONG
            bra         @cant

:
            MOVR        r0, tl_pp
            jsr         tl_readdir
            bcc         :+
@cant:
            MOVR        r0, tl_pp
            jmp         tl_err

:
            pha                                             ; This level's records: where, and how many
            txa
            ldx         tl_depth
            sta         tl_wlefth,X
            pla
            sta         tl_wleftl,X
            lda         r0
            sta         tl_wbasel,X
            sta         tl_wrecl,X
            lda         r0 + 1
            sta         tl_wbaseh,X
            sta         tl_wrech,X
            inc         tl_depth
@entry:
            ldx         tl_depth
            dex
            lda         tl_wleftl,X                         ; The next record, if there's one left
            ora         tl_wlefth,X
            beq         @done
            lda         tl_wleftl,X
            bne         :+
            dec         tl_wlefth,X
:
            dec         tl_wleftl,X
            lda         tl_wrecl,X
            sta         r0
            clc
            adc         #SR_SIZE
            sta         tl_wrecl,X
            lda         tl_wrech,X
            sta         r0 + 1
            adc         #0
            sta         tl_wrech,X
            jsr         tl_pcat                             ; Its path (its name: the record's start)
            bcc         :+
            jsr         @cant
            bra         @entry

:
            ldx         tl_depth
            sta         tl_wplen - 1,X
            ldy         #SR_QTYPE
            lda         (r0),Y
            and         #QT_DIR
            beq         @file
            jsr         @dir                                ; A directory: it, the tree in it, it again
            bcs         @next
            jsr         tl_walk
            ldx         tl_depth                            ; (Its record again: the one before the next)
            sec
            lda         tl_wrecl - 1,X
            sbc         #SR_SIZE
            sta         r0
            lda         tl_wrech - 1,X
            sbc         #0
            sta         r0 + 1
            jsr         @post
            bra         @next

@file:
            jsr         @vfile
@next:
            ldx         tl_depth
            lda         tl_wplen - 1,X
            jsr         tl_pcut
            bra         @entry

@done:
            lda         tl_wbasel,X                         ; Its records given back
            sta         r0
            lda         tl_wbaseh,X
            sta         r0 + 1
            dec         tl_depth
            jmp         tl_free

@dir:
            jmp         (tl_vdir)

@post:
            jmp         (tl_vpost)

@vfile:
            jmp         (tl_vfile)

; ****************************************************************************
; Input: a file (or fd 0), 256 bytes at a time

; The file named at r0 for tl_getc (- or none, r0 = 0: fd 0).  OUT: C = 0; or C = 1, .A = the error
tl_inopen:
            stz         tl_ipos
            stz         tl_ilen
            lda         r0
            ora         r0 + 1
            beq         @stdin
            lda         (r0)                                ; ("-": fd 0 too)
            cmp         #'-'
            bne         :+
            ldy         #1
            lda         (r0),Y
            beq         @stdin
:
            lda         #O_READ
            jsr         OPEN
            bcs         @done
            sta         tl_ifd
            clc
@done:
            rts

@stdin:
            lda         #$FF                                ; (Not to be closed)
            sta         tl_ifd
            clc
            rts

; The input's next byte: .A; C = 1 at its end (an error: said, about the name at tl_iname).  Keeps .X, .Y
tl_getc:
            phx
            ldx         tl_ipos
            cpx         tl_ilen
            bcc         @have
            phy                                             ; (Empty: 255 more, at most)
            lda         r0
            pha
            lda         r0 + 1
            pha
            lda         r1
            pha
            lda         r1 + 1
            pha
            LDR         r0, tl_ibuf
            LDR         r1, 255
            lda         tl_ifd
            bpl         :+
            lda         #0                                  ; (fd 0)
:
            jsr         READ
            bcc         :+
            pha                                             ; (An error: said, and the end)
            MOVR        r0, tl_iname
            pla
            jsr         tl_err
            lda         #0
:
            sta         tl_ilen
            stz         tl_ipos
            pla
            sta         r1 + 1
            pla
            sta         r1
            pla
            sta         r0 + 1
            pla
            sta         r0
            ply
            ldx         #0
            lda         tl_ilen
            bne         @have
            plx
            sec
            rts

@have:
            lda         tl_ibuf,X
            inc         tl_ipos
            plx
            clc
            rts

; The input closed (unless it's fd 0)
tl_inclose:
            lda         tl_ifd
            bmi         :+
            jsr         CLOSE
:
            rts

; Each input named from tl_arg on (none: fd 0) given in turn to the tool's routine (r0: tl_ivec), the input open
; for tl_getc and tl_iname its name (for errors).  One that can't be opened is said (tl_err)
tl_eachin:
            lda         (tl_arg)
            bne         @arg
            LDR         tl_iname, tl_s_stdin
            stz         r0
            stz         r0 + 1
            jsr         tl_inopen
            jmp         (tl_ivec)

@arg:
            MOVR        tl_iname, tl_arg
            MOVR        r0, tl_arg
            jsr         tl_inopen
            bcc         :+
            pha
            MOVR        r0, tl_arg
            pla
            jsr         tl_err
            bra         @next

:
            jsr         @call
            jsr         tl_inclose
@next:
            jsr         tl_next
            bne         @arg
            rts

@call:
            jmp         (tl_ivec)

; ****************************************************************************
; Numbers and times

; tl_num = the decimal number at r0.  OUT: C = 0; or C = 1 (not one: empty, or not digits alone)
tl_atoi:
            stz         tl_num
            stz         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            ldy         #0
            lda         (r0)
            beq         @bad
@digit:
            lda         (r0),Y
            beq         @done
            sec
            sbc         #'0'
            cmp         #10
            bcs         @bad
            pha
            ldx         #3                                  ; tl_num * 10: * 2 into tl_t, * 8, and the two added
:
            lda         tl_num,X
            sta         tl_t,X
            dex
            bpl         :-
            ldx         #3
:
            asl         tl_num
            rol         tl_num + 1
            rol         tl_num + 2
            rol         tl_num + 3
            cpx         #3
            bne         :+
            asl         tl_t
            rol         tl_t + 1
            rol         tl_t + 2
            rol         tl_t + 3
:
            dex
            bne         :--
            clc
            lda         tl_num
            adc         tl_t
            sta         tl_num
            lda         tl_num + 1
            adc         tl_t + 1
            sta         tl_num + 1
            lda         tl_num + 2
            adc         tl_t + 2
            sta         tl_num + 2
            lda         tl_num + 3
            adc         tl_t + 3
            sta         tl_num + 3
            pla                                             ; And the digit
            clc
            adc         tl_num
            sta         tl_num
            bcc         :+
            inc         tl_num + 1
            bne         :+
            inc         tl_num + 2
            bne         :+
            inc         tl_num + 3
:
            iny
            bne         @digit
@done:
            clc
            rts

@bad:
            sec
            rts

; tl_num = tl_num / tl_den, tl_rem the remainder (4 bytes each)
tl_div:
            stz         tl_rem
            stz         tl_rem + 1
            stz         tl_rem + 2
            stz         tl_rem + 3
            ldx         #32
@bit:
            asl         tl_num                              ; The next bit into the remainder
            rol         tl_num + 1
            rol         tl_num + 2
            rol         tl_num + 3
            rol         tl_rem
            rol         tl_rem + 1
            rol         tl_rem + 2
            rol         tl_rem + 3
            sec                                             ; The divisor out of it, if it goes
            lda         tl_rem
            sbc         tl_den
            sta         tl_t
            lda         tl_rem + 1
            sbc         tl_den + 1
            sta         tl_t + 1
            lda         tl_rem + 2
            sbc         tl_den + 2
            sta         tl_t + 2
            lda         tl_rem + 3
            sbc         tl_den + 3
            bcc         @next
            sta         tl_rem + 3
            lda         tl_t + 2
            sta         tl_rem + 2
            lda         tl_t + 1
            sta         tl_rem + 1
            lda         tl_t
            sta         tl_rem
            inc         tl_num                              ; (A 1 in the quotient: its low bit was 0)
@next:
            dex
            bne         @bit
            rts

; The time at r0 (seconds since 2000-01-01 00:00) into the output: "2000-01-31 12:59"
tl_date:
            ldy         #3
:
            lda         (r0),Y
            sta         tl_num,Y
            dey
            bpl         :-
            lda         #<86400                             ; Its days, and the seconds into the last
            ldx         #>86400
            ldy         #^86400
            jsr         tl_by
            lda         tl_rem                              ; (Those seconds, for later)
            pha
            lda         tl_rem + 1
            pha
            lda         tl_rem + 2
            pha
            LDR         tl_year, 2000                       ; The year: whole years off the days
@year:
            ldx         #<365
            lda         tl_year
            and         #3                                  ; (A leap year: every 4th, to 2099)
            bne         :+
            ldx         #<366
:
            stx         tl_t
            sec
            lda         tl_num
            sbc         tl_t
            sta         tl_t
            lda         tl_num + 1
            sbc         #>365
            bcc         @month
            sta         tl_num + 1
            lda         tl_t
            sta         tl_num
            inc         tl_year
            bne         @year
            inc         tl_year + 1
            bra         @year

@month:
            stz         tl_month                            ; The month: whole months off the days
@mon:
            ldx         tl_month
            ldy         tl_mdays,X                          ; (.Y: its days)
            cpx         #1                                  ; (February in a leap year: 29)
            bne         :+
            lda         tl_year
            and         #3
            bne         :+
            iny
:
            sty         tl_t
            lda         tl_num + 1                          ; Fewer days left than it has: it's this one
            bne         :+
            lda         tl_num
            cmp         tl_t
            bcc         @day
:
            sec
            lda         tl_num
            sbc         tl_t
            sta         tl_num
            lda         tl_num + 1
            sbc         #0
            sta         tl_num + 1
            inc         tl_month
            bra         @mon

@day:
            inc         tl_num                              ; (1 on)
            lda         tl_num                              ; Its day ...
            pha
            lda         tl_year                             ; "YYYY-MM-DD"
            sta         tl_num
            lda         tl_year + 1
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            lda         #4
            jsr         tl_dec
            lda         #'-'
            jsr         tl_putc
            lda         tl_month
            inc         a
            jsr         tl_two
            lda         #'-'
            jsr         tl_putc
            pla
            jsr         tl_two
            jsr         tl_space
            pla                                             ; "HH:MM", from the seconds
            sta         tl_num + 2
            pla
            sta         tl_num + 1
            pla
            sta         tl_num
            stz         tl_num + 3
            lda         #<3600
            ldx         #>3600
            ldy         #0
            jsr         tl_by
            lda         tl_num
            jsr         tl_two
            lda         #':'
            jsr         tl_putc
            lda         tl_rem
            sta         tl_num
            lda         tl_rem + 1
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            lda         #60
            ldx         #0
            ldy         #0
            jsr         tl_by
            lda         tl_num
; .A (0-99) into the output in two digits
tl_two:
            ldx         #'0'
:
            cmp         #10
            bcc         :+
            sbc         #10
            inx
            bra         :-
:
            pha
            txa
            jsr         tl_putc
            pla
            ora         #'0'
            jmp         tl_putc

; tl_num divided by .A/.X/.Y (low, middle, high byte: under 16M)
tl_by:
            sta         tl_den
            stx         tl_den + 1
            sty         tl_den + 2
            stz         tl_den + 3
            jmp         tl_div

.rodata
tl_bits:    .byte       $01, $02, $04, $08, $10, $20, $40, $80
tl_mdays:   .byte       31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
tl_s_usage: .byte       "usage: ", 0
tl_s_ustat: .byte       "usage", 0
tl_s_werr:  .byte       "write error", 0
tl_s_colon: .byte       ": ", 0
tl_s_nl:    .byte       LF, 0
tl_s_stdin: .byte       "-", 0

; The powers of 10, 10^9 first, a byte of each a table
.macro TL_P10 shift
            .byte       (1000000000 >> shift) & $FF, (100000000 >> shift) & $FF, (10000000 >> shift) & $FF
            .byte       (1000000 >> shift) & $FF, (100000 >> shift) & $FF, (10000 >> shift) & $FF
            .byte       (1000 >> shift) & $FF, (100 >> shift) & $FF, (10 >> shift) & $FF, 1 >> shift
.endmacro
tl_p10_0:   TL_P10      0
tl_p10_1:   TL_P10      8
tl_p10_2:   TL_P10      16
tl_p10_3:   TL_P10      24

.popseg
