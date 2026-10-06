; ****************************************************************************
; streams.inc - hylang's streams (phase 7), in its fifth bank, as danlang's (LStream.cs, Builtins.cs): open, close,
; read-line, read-byte, read-all, seek, tell, print-to, write-to, read-bytes, write-bytes; stdin, stdout and stderr;
; save and read.  A stream (PK_STREAM) is its fd and its flags, a word: a file's (open's), read or written, or the
; console's.  stdin is read through the REPL's own buffer (hylang.s: getc_in), so a program's reads and the REPL's
; share it; stdout is the output (out.inc), so output-of has what's printed to it; stderr is fd 2.  A line ends at
; LF (a CR before it dropped); a file's line is read a chunk at a time, what's past it given back (SEEK).

SM_IN           = $01           ; (A stream's flags, its word's high byte: it's read ...
SM_OUT          = $02           ;   written ...
SM_CON          = $04           ;   the console's (not closed) ...
SM_SHUT         = $08           ;   closed
SM_STDOUT       = $10           ;   stdout: the output itself)
CHUNK           = 64            ; (A line's bytes read at a time)

.bss
y_sfd:      .res        1                                   ; (The stream argument: its fd, its flags, its cell)
y_sfl:      .res        1
y_scell:    .res        2

.segment "CODE5"

; ---- Streams made

; stdin, stdout and stderr: streams of fds 0, 1 and 2, bound; as hylang starts without a snapshot (a snapshot has
; them).  OUT: C = 1 if there's no room
y_std:
            jsr         y_enter
            ldx         #0
@one:
            phx
            lda         con_fd,x
            sta         y_sfd
            lda         con_fl,x
            ldx         y_sfd
            jsr         y_stream
            plx
            phx
            lda         con_name_lo,x
            pha
            lda         con_name_hi,x
            tax
            pla
            jsr         y_bind
            plx
            inx
            cpx         #3
            bcc         @one
            clc
            rts

; args: the arguments at hl_argp (zero-terminated, an empty one after the last; hl_argp 0, none), a list of strings;
; as hylang starts.  OUT: C = 1 if there's no room; hv = (load path), what a script runs (its path args' first), or
; NIL if there's none
y_init:
            jsr         y_enter
            stz         cf                                  ; (clock.start: (ticks) and (time) as hylang starts,
            stz         cf + 1                              ;   hylib.hl's clock's)
            jsr         TICKS
            sta         n_w32
            txa
            and         #$7F
            sta         n_w32 + 1
            stz         n_w32 + 2
            stz         n_w32 + 3
            jsr         y_u32
            lda         hv
            ldx         hv + 1
            jsr         y_append
            jsr         TIME
            ldx         #3
:
            lda         r0,x
            sta         n_w32,x
            dex
            bpl         :-
            jsr         y_u32
            lda         hv
            ldx         hv + 1
            jsr         y_append
            MOVW        hv, cf
            LDAX        s_clockst
            jsr         y_bind
            stz         cf                                  ; (args: cf)
            stz         cf + 1
            MOVW        nb, hl_argp
            lda         nb
            ora         nb + 1
            beq         @made
@arg:
            lda         (nb)
            beq         @made
            ldy         #0
:
            lda         (nb),y
            beq         :+
            iny
            bne         :-
:
            sty         hn
            stz         hn + 1
            MOVW        hq, nb
            phy
            jsr         string_make
            ply
            bcc         :+
            jmp         y_nomem
:
            tya
            sec
            adc         nb
            sta         nb
            bcc         :+
            inc         nb + 1
:
            lda         hv
            ldx         hv + 1
            jsr         y_append
            bra         @arg
@made:
            lda         cf                                  ; (args)
            sta         hv
            lda         cf + 1
            sta         hv + 1
            LDAX        s_args
            jsr         y_bind
            stz         hv                                  ; (A script: (load path))
            stz         hv + 1
            lda         cf
            ora         cf + 1
            beq         @done
            lda         cf
            ldx         cf + 1
            ldy         #0
            jsr         cell_get
            sta         et
            stx         et + 1
            stz         eu
            stz         eu + 1
            lda         #PK_SCONS
            jsr         y_make2
            MOVW        eu, hv
            lda         #<(BUILTIN0 + 2 * BIN_LOAD)
            sta         et
            lda         #>(BUILTIN0 + 2 * BIN_LOAD)
            sta         et + 1
            lda         #PK_SCONS
            jsr         y_make2
@done:
            clc
            rts

; A script's error (ex) out to stderr: as the REPL shows it, and LF
y_errout:
            jsr         y_enter
            jsr         y_begin
            lda         ex
            ldx         ex + 1
            ldy         #0
            FARN        2, print_val
            lda         #LF
            jsr         out_byte
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            jsr         out_flush
            lda         #2
            sta         y_fd
            lda         hv
            ldx         hv + 1
            jmp         y_spill

; ex = the system's error .A about cf (a path load didn't find), for the first bank.  OUT: C = 1 if there's no room
y_loaderr:
            sta         y_c
            jsr         y_enter
            MOVW        y_name, cf
            lda         y_c
            jmp         y_syserr

; hv = a stream of the fd .X, flags .A
y_stream:
            pha
            phx
            lda         #PK_STREAM
            jsr         y_alloc
            pla
            sta         (hp)
            pla
            ldy         #1
            sta         (hp),y
            rts

; The symbol named at .A, .X (zero-terminated, in this bank) given the global value hv
y_bind:
            pha
            phx
            lda         hv
            ldx         hv + 1
            jsr         y_rpush
            plx
            pla
            sta         hq
            stx         hq + 1
            ldy         #0
:
            lda         (hq),y
            beq         :+
            iny
            bne         :-
:
            sty         hn
            stz         hn + 1
            lda         #PK_SYMBOL
            jsr         intern
            bcc         :+
            jmp         y_nomem
:
            jsr         root_pop
            ldy         #1                                  ; (Its global value)
            jmp         cell_set

; ---- The stream argument

; The argument at .Y a stream: y_scell, y_sfd, y_sfl.  OUT: C = 1 and ex = the error "'fn' expects a stream" (or,
; y_stream_first, "... a stream first") if it isn't one
y_streamarg:
            LDAX        s_ystream
            bra         y_starg
y_stream_first:
            LDAX        s_streamfirst
            ldy         #0
y_starg:
            sta         y_w
            stx         y_w + 1
            jsr         y_argy
            sta         y_scell
            stx         y_scell + 1
            jsr         kind_of
            cmp         #PK_STREAM
            beq         :+
            lda         y_w
            ldx         y_w + 1
            jsr         y_fnerr0
            sec
            rts
:
            lda         y_scell
            ldx         y_scell + 1
            ldy         #0
            jsr         cell_get
            sta         y_sfd
            stx         y_sfl
            clc
            rts

; ex = the error "'fn' message", with no code (y_fnerr's, but danlang's own stream messages)
y_fnerr0:
            pha
            phx
            stz         y_code
            stz         y_code + 1
            jsr         y_begin
            lda         #$27
            jsr         out_byte
            ldx         bi_n
            FARN        2, bi_name
            plx
            pla
            jsr         out_text
            jmp         y_end

; Can the stream be read (y_canread), written (y_canwrite)?  OUT: C = 1 and ex = the error ("Cannot read from this
; stream", "Cannot write this stream") if it can't
y_canread:
            lda         #SM_IN
            ldx         #<s_noread
            ldy         #>s_noread
            bra         y_can
y_canwrite:
            lda         #SM_OUT
            ldx         #<s_nowrite
            ldy         #>s_nowrite
y_can:
            and         y_sfl
            beq         @no
            lda         y_sfl
            and         #SM_SHUT
            bne         @no
            clc
            rts
@no:
            txa
            phy
            plx
            jsr         y_error
            sec
            rts

; The stream's next byte.  OUT: C = 0, .A = it; C = 1: .A = 0 at its end, else the system's error
y_getb:
            lda         y_sfl
            and         #SM_CON
            beq         @file
            FARN        1, getc_in                          ; (stdin: the REPL's buffer)
            rts
@file:
            LDR         r0, y_buf
            LDR         r1, 1
            lda         y_sfd
            jsr         READ
            bcs         @rts
            cmp         #0
            beq         @end
            lda         y_buf
            clc
            rts
@end:
            lda         #0
            sec
@rts:
            rts

; ex = the system's error .A if it isn't 0 (else NIL: the stream's end); C = 0
y_endor:
            cmp         #0
            bne         :+
            jmp         y_nil
:
            stz         y_name
            stz         y_name + 1
            jmp         y_syserr

; ---- The built-ins

; (open path [mode]): a stream on the file: :read (the default), :write (made, or emptied), :append or :update (read
; and written in place: it must be there, else :noent; nothing emptied)
bi_open:
            jsr         y_enter
            jsr         y_arg0                              ; (Its path: a string)
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            LDAX        s_yexpects
            jmp         y_fnerr0
:
            stz         y_c                                 ; (Its mode: 0 read, 1 write, 2 append, 3 update)
            lda         cn
            cmp         #2
            bcc         @path
            jsr         y_arg1
            sta         y_v
            stx         y_v + 1
            jsr         kind_of
            cmp         #PK_ATOM
            beq         :+
            LDAX        s_openmode
            jmp         y_error
:
            ldx         #3
@mode:
            phx
            lda         mode_lo,x
            pha
            lda         mode_hi,x
            tax
            pla
            jsr         y_atom
            plx
            lda         hv
            cmp         y_v
            bne         :+
            lda         hv + 1
            cmp         y_v + 1
            beq         @found
:
            dex
            bpl         @mode
            jsr         y_begin                             ; ("'open' mode must be ..., not :sideways")
            LDAX        s_openmode
            jsr         out_text
            LDAX        s_ynot
            jsr         out_text
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            FARN        2, print_val
            jmp         y_end
@found:
            stx         y_c
@path:
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcc         :+
            clc
            rts
:
            jsr         y_stat                              ; (A directory: not a stream)
            bcs         @open
            jsr         y_isdir
            beq         @open
            lda         #E_ISDIR
            jmp         y_syserr0
@open:
            lda         #SM_IN                              ; (Its flags: y_sfl)
            sta         y_sfl
            lda         y_c
            bne         @write
            lda         #O_READ
            jsr         y_open
            bra         @made
@write:
            ldx         #SM_OUT
            stx         y_sfl
            cmp         #1
            bne         @append
            jsr         y_create
            bra         @made
@append:
            cmp         #3
            beq         @update
            jsr         y_append_to
            bra         @made
@update:
            lda         #SM_IN | SM_OUT                     ; (Read and written: as it is)
            sta         y_sfl
            lda         #O_RDWR
            jsr         y_open
@made:
            bcc         :+
            jmp         y_syserr0
:
            lda         y_sfl
            ldx         y_fd
            jsr         y_stream
            MOVW        ex, hv
            clc
            rts

; (close s): the stream closed (closing it again does nothing); NIL
bi_close:
            jsr         y_enter
            ldy         #0
            jsr         y_streamarg
            bcs         y_done
            lda         y_sfl
            and         #SM_CON
            beq         :+
            LDAX        s_closecon
            jmp         y_error
:
            lda         y_sfl
            and         #SM_SHUT
            bne         @done
            lda         y_sfd
            jsr         CLOSE
            lda         y_sfl
            ora         #SM_SHUT
            ldx         y_scell
            stx         hv
            ldx         y_scell + 1
            stx         hv + 1
            tax
            lda         y_sfd
            ldy         #0
            jsr         cell_set
@done:
            jmp         y_nil
y_done:
            clc
            rts

; The stream argument 0, or stdin if there's none (read-line's, read-byte's, read-all's), that can be read.  OUT:
; C = 1 and ex = the error if it isn't one
y_instream:
            lda         cn
            bne         :+
            stz         y_sfd                               ; (stdin)
            lda         #SM_IN | SM_CON
            sta         y_sfl
            clc
            rts
:
            ldy         #0
            jsr         y_streamarg
            bcs         :+
            jsr         y_canread
:
            rts

; (read-line [s]): its next line (without its end), or NIL at its end
bi_read_line:
            jsr         y_enter
            jsr         y_instream
            bcs         y_done
            jsr         y_begin
            stz         y_n                                 ; (Bytes in it so far: y_n; none: NIL at the end)
            stz         y_n + 1
            stz         y_cr
            lda         y_sfl
            and         #SM_CON
            beq         @chunk
            FARN        1, raw_off                          ; (stdin's: key's raw mode off, a line read)
            bra         @byte
@chunk:
            LDR         r0, y_buf                           ; (A file's: a chunk, what's past the line given back)
            LDR         r1, CHUNK
            lda         y_sfd
            jsr         READ
            bcs         @fail
            cmp         #0
            beq         @end
            sta         y_i
            ldx         #0
@scan:
            lda         y_buf,x
            inx
            cmp         #LF
            beq         @back
            jsr         @add
            cpx         y_i
            bne         @scan
            bra         @chunk
@back:
            txa                                             ; (The rest of the chunk: back)
            sec
            sbc         y_i
            beq         @line
            sta         r0
            lda         #$FF
            sta         r0 + 1
            sta         r1
            sta         r1 + 1
            ldx         #1
            lda         y_sfd
            jsr         SEEK
            bra         @line
@byte:
            jsr         y_getb                              ; (stdin's: a byte at a time)
            bcs         @stop
            cmp         #LF
            beq         @line
            jsr         @add
            bra         @byte
@stop:
            cmp         #0
            beq         @end
@fail:
            pha
            lda         y_start
            ldx         y_start + 1
            jsr         cap_drop
            pla
            jmp         y_endor
@end:
            lda         y_n                                 ; (Its end: NIL, if there's nothing)
            ora         y_n + 1
            bne         @line
            lda         y_start
            ldx         y_start + 1
            jsr         cap_drop
            jmp         y_nil
@line:
            lda         y_cr                                ; (A CR before its end: dropped)
            beq         :+
            dec         cap_len                             ; (It's the capture's last byte)
            lda         cap_len
            cmp         #$FF
            bne         :+
            dec         cap_len + 1
:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            MOVW        ex, hv
            clc
            rts
@add:
            stz         y_cr                                ; (A byte of it: out; y_cr, whether it's a CR)
            cmp         #CR
            bne         :+
            inc         y_cr
:
            jsr         out_byte
            inc         y_n
            bne         :+
            inc         y_n + 1
:
            rts

.bss
y_cr:       .res        1
.segment "CODE5"

; (read-byte [s]): its next byte (0-255), or NIL at its end
bi_read_byte:
            jsr         y_enter
            jsr         y_instream
            bcs         y_done2
            jsr         y_getb
            bcc         :+
            jmp         y_endor
:
            ldx         #0
            jmp         y_fix
y_done2:
            clc
            rts

; (read-all [s]): the rest of it, a string ("" at its end)
bi_read_all:
            jsr         y_enter
            jsr         y_instream
            bcs         y_done2
            lda         y_sfl
            and         #SM_CON
            bne         @con
            lda         y_sfd
            sta         y_fd
            jsr         y_slurp
            cmp         #0
            beq         y_done2
            jmp         y_endor
@con:
            jsr         y_begin
@byte:
            jsr         y_getb
            bcs         @end
            jsr         out_byte
            bra         @byte
@end:
            pha
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            MOVW        ex, hv
            pla
            cmp         #0
            beq         y_done2
            jmp         y_endor

; (tell s): its place (bytes from its start)
bi_tell:
            jsr         y_enter
            ldy         #0
            jsr         y_streamarg
            bcs         y_done2
            LDAX        s_nopos
            jsr         y_seekable
            bcs         y_done2
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #1
            lda         y_sfd
            jsr         SEEK
            bcc         y_place
            jmp         y_syserr0
; ex = the place SEEK gave (r0, r1)
y_place:
            ldx         #3
:
            lda         r0,x
            sta         n_w32,x
            dex
            bpl         :-
            jmp         y_u32

; Is the stream a file's, open (seekable)?  OUT: C = 1 and ex = the error .A, .X (seek's "Cannot seek on a
; non-seekable stream", tell's "Cannot get the position ...") if it isn't
y_seekable:
            pha
            lda         y_sfl
            and         #SM_CON | SM_SHUT
            bne         :+
            pla
            clc
            rts
:
            pla
            jsr         y_error
            sec
            rts

; (seek s n): its place set (0 to its length); the place
bi_seek:
            jsr         y_enter
            ldy         #0
            jsr         y_streamarg
            bcc         @lb409
            jmp         y_done3
@lb409:
            LDAX        s_noseek
            jsr         y_seekable
            bcc         @lb408
            jmp         y_done3
@lb408:
            jsr         y_arg1                              ; (n: a number, 0 to its length)
            sta         y_v
            stx         y_v + 1
            FARN        3, n_kind
            cmp         #NK_CPX
            bcc         :+
            LDAX        s_seeknum
            jmp         y_error
:
            lda         y_v
            ldx         y_v + 1
            FARN        3, n_sign3                          ; (0: below 0)
            cmp         #0
            bne         :+
            LDAX        s_seekneg
            jmp         y_error
:
            LDR         r0, y_buf                           ; (Its length)
            lda         y_sfd
            jsr         FSTAT
            bcc         :+
            jmp         y_syserr0
:
            lda         y_v
            ldx         y_v + 1
            FARN        3, n_to_u32
            bcs         @past
            ldx         #0                                  ; (Past its length: no)
            ldy         #4
            sec
:
            lda         y_buf + SR_LENGTH,x
            sbc         n_w32,x
            inx
            dey
            bne         :-
            bcc         @past
            ldx         #3
:
            lda         n_w32,x
            sta         r0,x
            dex
            bpl         :-
            ldx         #0
            lda         y_sfd
            jsr         SEEK
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_place
@past:
            LDAX        s_seekpast
            jmp         y_error
y_done3:
            clc
            rts

; (print-to s x...), (write-to s x...): print's and write's output, to the stream (stdout's: the output)
bi_print_to:
            lda         #1
            bra         y_outto
bi_write_to:
            lda         #0
y_outto:
            sta         y_c
            jsr         y_enter
            jsr         y_stream_first
            bcs         y_done3
            jsr         y_canwrite
            bcs         y_done3
            lda         y_sfl
            and         #SM_STDOUT
            beq         @capture
            jsr         y_items                             ; (stdout: the output, as print's)
            jmp         y_nil
@capture:
            jsr         y_begin                             ; (Else a string of them, written)
            jsr         y_items
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            lda         hv
            ldx         hv + 1
            jsr         y_rpush
            jsr         out_flush                           ; (What's out first: stderr's after it)
            lda         y_sfd
            sta         y_fd
            jsr         root_pop
            jsr         y_spill
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_nil

; The arguments from 1 out, as print shows them: each after a space (print's, y_c 1) or none (write's), then LF
; (print's)
y_items:
            lda         #1
            sta         y_n
@item:
            lda         y_n
            cmp         cn
            bcs         @end
            cmp         #2
            bcc         :+
            lda         y_c
            beq         :+
            lda         #' '
            jsr         out_byte
:
            lda         y_n
            asl
            tay
            jsr         y_argy
            ldy         #1
            FARN        2, print_val
            inc         y_n
            bra         @item
@end:
            lda         y_c
            beq         :+
            lda         #LF
            jsr         out_byte
:
            rts

; (read-bytes s n): its next n bytes, a list (fewer at its end; NIL if there are none)
bi_read_bytes:
            jsr         y_enter
            ldy         #0
            jsr         y_streamarg
            bcs         y_done4
            jsr         y_canread
            bcs         y_done4
            jsr         y_arg1                              ; (n: an integer)
            FARN        3, n_to_u32
            bcc         :+
            LDAX        s_count
            jmp         y_inval
:
            stz         cf
            stz         cf + 1
@byte:
            lda         n_w32                               ; (n_w32: the bytes still wanted)
            ora         n_w32 + 1
            ora         n_w32 + 2
            ora         n_w32 + 3
            beq         @done
            jsr         y_getb
            bcs         @end
            ldx         #0
            jsr         fix_ax
            jsr         y_append
            ldx         #0
:
            lda         n_w32,x
            dec         n_w32,x
            cmp         #0
            bne         @byte
            inx
            cpx         #4
            bne         :-
            bra         @byte
@end:
            cmp         #0
            beq         @done
            jmp         y_endor
@done:
            MOVW        ex, cf
y_done4:
            clc
            rts

; (write-bytes s l), (write-bytes s b [i [n]]): the bytes (a list of integers, 0-255; a buffer's from i (0) on, n of
; them (to its end): buffers.inc's) written as they are
bi_write_bytes:
            jsr         y_enter
            ldy         #0
            jsr         y_streamarg
            bcs         y_done4
            jsr         y_canwrite
            bcs         y_done4
            jsr         y_arg1
            jsr         kind_of
            cmp         #PK_BUFFER
            bne         :+
            FARN        6, bf_wbytes                        ; (Its bytes, as a string's are: y_begin's)
            bra         @write
:
            lda         cn                                  ; (A list: no part of it)
            cmp         #3
            bcc         :+
            LDAX        s_wpart
            jmp         y_error
:
            jsr         y_arg1
            jsr         q_conses
            bcc         :+
            LDAX        s_wbytes
            jmp         y_fnerr
:
            sta         y_v
            stx         y_v + 1
            jsr         y_begin                             ; (A string of them, written)
@byte:
            lda         y_v
            ora         y_v + 1
            beq         @write
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            sta         y_w
            stx         y_w + 1
            FARN        3, n_to_u32
            bcs         @notint
            lda         n_w32 + 1
            ora         n_w32 + 2
            ora         n_w32 + 3
            beq         @lb407
            jmp         @range
@lb407:
            lda         n_w32
            jsr         out_byte
            lda         y_v
            ldx         y_v + 1
            ldy         #1
            jsr         cell_get
            sta         y_v
            stx         y_v + 1
            bra         @byte
@write:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            lda         y_sfl
            and         #SM_STDOUT
            beq         :+
            lda         hv                                  ; (stdout: out)
            ldx         hv + 1
            ldy         #1
            FARN        2, print_val
            jmp         y_nil
:
            lda         hv
            ldx         hv + 1
            jsr         y_rpush
            jsr         out_flush
            lda         y_sfd
            sta         y_fd
            jsr         root_pop
            jsr         y_spill
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_nil
@notint:
            lda         y_w                                 ; (A negative integer: "-1 isn't a byte")
            ldx         y_w + 1
            FARN        3, n_kind
            bne         @abyte
@range:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_drop
            lda         #E_INVAL
            jsr         y_errcode
            jsr         y_begin
            lda         y_w
            ldx         y_w + 1
            ldy         #0
            FARN        2, print_val
            LDAX        s_ynotbyte
            jsr         out_text
            jmp         y_end
@abyte:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_drop
            LDAX        s_yabyte
            jmp         y_inval

; (save x [path mode...]): x as it reads back, printed (with LF), or written to the file (:overwrite or :append it,
; if it's there); NIL
bi_save:
            jsr         y_enter
            lda         cn
            cmp         #2
            bcs         @file
            jsr         y_arg0                              ; (Printed)
            ldy         #2
            FARN        2, print_val
            lda         #LF
            jsr         out_byte
            jmp         y_nil
@file:
            ldy         #2                                  ; (Its path: a string)
            jsr         y_argy
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            LDAX        s_save2
            jmp         y_error
:
            stz         y_c                                 ; (Its mode: bit 0 overwrite, bit 1 append)
            lda         #2
            sta         y_n
@mode:
            lda         y_n
            cmp         cn
            bcs         @path
            asl
            tay
            jsr         y_argy
            sta         y_v
            stx         y_v + 1
            jsr         kind_of
            cmp         #PK_ATOM
            beq         :+
            LDAX        s_save3
            jmp         y_error
:
            LDAX        s_overwrite
            jsr         y_atom
            lda         hv
            cmp         y_v
            bne         :+
            lda         hv + 1
            cmp         y_v + 1
            bne         :+
            lda         #1
            tsb         y_c
:
            LDAX        s_append
            jsr         y_atom
            lda         hv
            cmp         y_v
            bne         :+
            lda         hv + 1
            cmp         y_v + 1
            bne         :+
            lda         #2
            tsb         y_c
:
            inc         y_n
            bra         @mode
@path:
            ldy         #2
            ldx         #0
            jsr         y_patharg
            bcc         :+
            clc
            rts
:
            jsr         y_stat                              ; (There: overwritten, or appended to)
            bcs         @new
            lda         y_c
            lsr
            bcc         :+
            jsr         y_create
            bra         @opened
:
            lsr
            bcs         :+
            LDAX        s_saveboth
            jmp         y_error
:
            jsr         y_append_to
            bra         @opened
@new:
            jsr         y_create
@opened:
            bcc         :+
            jmp         y_syserr1
:
            jsr         y_begin                             ; (x as it reads back, and LF: a string, written)
            jsr         y_arg0
            ldy         #2
            FARN        2, print_val
            lda         #LF
            jsr         out_byte
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            lda         hv
            ldx         hv + 1
            jsr         y_spill
            php
            pha
            lda         y_fd
            jsr         CLOSE
            pla
            plp
            bcc         :+
            jmp         y_syserr1
:
            jmp         y_nil

; ex = the system's error .A about the argument 1
y_syserr1:
            pha
            jsr         y_arg1
            sta         y_name
            stx         y_name + 1
            pla
            jmp         y_syserr

; (read text): the expressions in the text, unevaluated, a list: (eval (read "+ 1 2")) is 3
bi_read:
            jsr         y_enter
            jsr         y_arg0
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            LDAX        s_yexpects
            jmp         y_fnerr0
:
            jsr         y_arg0                              ; (Its bytes: the reader's text)
            jsr         deref
            lda         (hp)
            sta         t_len
            ldy         #1
            lda         (hp),y
            sta         t_len + 1
            cmp         #>TEXT_SIZE
            bcc         :+
            LDAX        s_readlong
            jmp         y_error
:
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            MOVW        y_i, t_len
            LDAX        text
            sta         nb
            stx         nb + 1
@copy:
            lda         y_i
            ora         y_i + 1
            beq         @copied
            lda         (hp)
            sta         (nb)
            inc         hp
            bne         :+
            inc         hp + 1
:
            inc         nb
            bne         :+
            inc         nb + 1
:
            lda         y_i
            bne         :+
            dec         y_i + 1
:
            dec         y_i
            bra         @copy
@copied:
            lda         #0
            sta         (nb)
            FARN        2, read_text
            bcc         :+
            jmp         y_nomem
:
            stz         t_len                               ; (The text no one's now)
            stz         t_len + 1
            cmp         #RD_DONE
            beq         @done
            cmp         #RD_MORE
            beq         @more
            jsr         y_begin                             ; ("'read': " and its error's message)
            LDAX        s_readerr
            jsr         out_text
            lda         hv
            ldx         hv + 1
            ldy         #0
            jsr         cell_get
            ldy         #1
            FARN        2, print_val
            jmp         y_end
@more:
            jsr         y_begin                             ; ("'read': missing )")
            LDAX        s_readmiss
            jsr         out_text
            ldx         #0
:
            cpx         cl_len
            bcs         :+
            lda         closers,x
            jsr         out_byte
            inx
            bra         :-
:
            jmp         y_end
@done:
            lda         hv                                  ; (Its items, a Q-expression's list; it kept meanwhile)
            ldx         hv + 1
            jsr         y_rpush
            stz         cf
            stz         cf + 1
            lda         hv
            ldx         hv + 1
@item:
            sta         y_v
            stx         y_v + 1
            jsr         list_kind
            cmp         #PK_SCONS
            bne         @made
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            jsr         y_append
            lda         y_v
            ldx         y_v + 1
            ldy         #1
            jsr         cell_get
            bra         @item
@made:
            dec         rsp
            MOVW        ex, cf
            clc
            rts

.segment "RODATA5"
con_fd:     .byte       0, 1, 2
con_fl:     .byte       SM_IN | SM_CON, SM_OUT | SM_CON | SM_STDOUT, SM_OUT | SM_CON
con_name_lo: .lobytes   s_stdin, s_stdout, s_stderr
con_name_hi: .hibytes   s_stdin, s_stdout, s_stderr
s_stdin:    .byte       "stdin", 0
s_stdout:   .byte       "stdout", 0
s_stderr:   .byte       "stderr", 0
s_args:     .byte       "args", 0
s_clockst:  .byte       "clock.start", 0
mode_lo:    .lobytes    s_mread, s_mwrite, s_mappend, s_mupdate
mode_hi:    .hibytes    s_mread, s_mwrite, s_mappend, s_mupdate
s_mread:    .byte       "read", 0
s_mwrite:   .byte       "write", 0
s_mappend:  .byte       "append", 0
s_mupdate:  .byte       "update", 0
s_overwrite: .byte      "overwrite", 0
s_append    = s_mappend
s_ystream:   .byte       "' expects a stream", 0
s_streamfirst: .byte    "' expects a stream first", 0
s_yexpects: .byte       "' expects a String", 0
s_wpart:    .byte       "'write-bytes' takes a part (i n) only of a buffer", 0
s_openmode: .byte       "'open' mode must be :read, :write, :append or :update", 0
s_ynot:      .byte       ", not ", 0
s_closecon: .byte       "Cannot close the console's streams", 0
s_noread:   .byte       "Cannot read from this stream", 0
s_nowrite:  .byte       "Cannot write this stream", 0
s_noseek:   .byte       "Cannot seek on a non-seekable stream", 0
s_nopos:    .byte       "Cannot get the position of a non-seekable stream", 0
s_seeknum:  .byte       "Invalid parameter 'byteOffset': is not a number", 0
s_seekneg:  .byte       "Cannot seek before beginning of stream", 0
s_seekpast: .byte       "Cannot seek past end of stream", 0
s_count:    .byte       "A count must be an integer", 0
s_wbytes:   .byte       "' expects a list of bytes", 0
s_ynotbyte:  .byte       " isn't a byte", 0
s_yabyte:    .byte       "a byte must be an integer", 0
s_save2:    .byte       "Second parameter to 'save' must be a string", 0
s_save3:    .byte       "Parameters 3+ for function 'save' must be atoms", 0
s_saveboth: .byte       "File exists but neither ':append' nor ':overwrite' were specified", 0
s_readlong: .byte       "'read': Too long: a text of more than 4095 bytes", 0
s_readerr:  .byte       "'read': ", 0
s_readmiss: .byte       "'read': missing ", 0

.segment "CODE5"
