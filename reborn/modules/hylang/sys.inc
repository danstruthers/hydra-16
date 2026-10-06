; ****************************************************************************
; sys.inc - hylang's streams and system library (phase 7), in its fifth bank: what its built-ins share.  An abort
; point (y_enter: no room for what's made goes back to it, y_nomem, the root stack and the captures as they were);
; errors, and the system's: "name: text" (ERRSTR's), its code an atom (error-code's :noent ..., the names of
; spec/errors.def: obj/gen/errnames.inc); a path argument (a string, or an atom's or a symbol's name: in RAM, with a
; 0 after it); a file's bytes read whole, a string's written.  A built-in's value is ex: what it gives, or an error.

.include "errnames.inc"

.bss
y_sp:       .res        1                                   ; The abort point: the 65C02's stack, the root stack, the
y_rsp:      .res        1                                   ;   captures open and their length
y_cap:      .res        1
y_clen:     .res        2
y_start:    .res        2                                   ; (A capture: where it began)
y_code:     .res        2                                   ; (An error's code: an atom, or NIL)
y_path:     .res        PATH_MAX + 1                        ; Two paths (y_patharg's)
y_path2:    .res        PATH_MAX + 1
y_buf:      .res        256                                 ; A file's bytes, a stat record, an error's text ...
y_fd:       .res        1                                   ; (A file: its fd)
y_n:        .res        2                                   ; (Counts, places)
y_i:        .res        2
y_v:        .res        2                                   ; (A value; a list walked)
y_w:        .res        2
y_nc:       .res        2                                   ; (y_append's: the cons made)
y_c:        .res        1                                   ; (A byte; a built-in's own)
y_slot:     .res        1                                   ; (y_patharg's)

.segment "CODE5"

; ---- Abort point, helpers

; An entry's abort point (first thing in it: jsr y_enter)
y_enter:
            tsx
            inx
            inx
            stx         y_sp
            lda         rsp
            sta         y_rsp
            lda         cap_on
            sta         y_cap
            lda         cap_len
            sta         y_clen
            lda         cap_len + 1
            sta         y_clen + 1
            stz         y_code
            stz         y_code + 1
            rts

; No room: back from the entry, C = 1, the root stack and the captures as they were
y_nomem:
            ldx         y_sp
            txs
            lda         y_rsp
            sta         rsp
            lda         y_cap
            sta         cap_on
            lda         y_clen
            sta         cap_len
            lda         y_clen + 1
            sta         cap_len + 1
            sec
            rts

; make2 (y_make2), root_push (y_rpush), cell_alloc (y_alloc): no room, back from the entry
y_make2:
            jsr         make2
            bcs         y_nomem
            rts
y_rpush:
            jsr         root_push
            bcs         y_nomem
            rts
y_alloc:
            jsr         cell_alloc
            bcs         y_nomem
            rts

; .A, .X put at the end of the list made (cf its first cons, cr its last)
y_append:
            sta         et
            stx         et + 1
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         y_make2
            lda         cf
            ora         cf + 1
            bne         :+
            MOVW        cf, hv
            MOVW        cr, hv
            rts
:
            MOVW        y_nc, hv
            MOVW        hv, cr
            lda         y_nc
            ldx         y_nc + 1
            ldy         #1
            jsr         cell_set
            MOVW        cr, y_nc
            rts

; hv = the list {et eu}
y_pair:
            lda         et
            pha
            lda         et + 1
            pha
            MOVW        et, eu
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         y_make2
            pla
            sta         et + 1
            pla
            sta         et
            MOVW        eu, hv
            lda         #PK_QCONS
            jmp         y_make2

; hv = the atom of the zero-terminated name at .A, .X (in this bank)
y_atom:
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
            lda         #PK_ATOM
            jsr         intern
            bcc         @lb406
            jmp         y_nomem
@lb406:
            rts

; The entry {:name value} put at the end of the list made (cf, cr): the name's at .A, .X (zero-terminated), the
; value hv's (kept on the root stack while the name's interned)
y_entry:
            pha
            phx
            lda         hv
            ldx         hv + 1
            jsr         y_rpush
            plx
            pla
            jsr         y_atom
            MOVW        et, hv
            jsr         root_pop
            sta         eu
            stx         eu + 1
            jsr         y_pair
            lda         hv
            ldx         hv + 1
            jmp         y_append

; .A, .X = the argument 0 (y_arg0), 1 (y_arg1), at .Y (y_argy)
y_arg0:
            ldy         #0
            bra         y_argy
y_arg1:
            ldy         #2
y_argy:
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            rts

; ex = NIL (y_nil) or T (y_t); C = 0
y_nil:
            stz         ex
            stz         ex + 1
            clc
            rts
y_t:
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            clc
            rts

; ex = the integer .A, .X (0-16383); C = 0
y_fix:
            jsr         fix_ax
            sta         ex
            stx         ex + 1
            clc
            rts

; ex = the integer of the four bytes at n_w32 (least first; unsigned); C = 0
y_u32:
            FARN        3, n_u32
            bcc         @lb405
            jmp         y_nomem
@lb405:
            MOVW        ex, hv
            clc
            rts

; ---- Errors

; A message begun (y_end: ended, ex the error, its code y_code, C = 0)
y_begin:
            jsr         cap_begin
            sta         y_start
            stx         y_start + 1
            rts
y_end:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         @lb404
            jmp         y_nomem
@lb404:
            lda         hv
            ldx         hv + 1
            jsr         kind_of
            cmp         #PK_ERROR
            beq         :+
            jsr         error_wrap
            bcc         @lb403
            jmp         y_nomem
@lb403:
            lda         y_code                              ; (Its code)
            ldx         y_code + 1
            ldy         #1
            jsr         cell_set
:
            MOVW        ex, hv
            clc
            rts

; ex = the error of the zero-terminated message at .A, .X (in this bank), its code :inval (y_inval) or none
; (y_error); C = 0
y_inval:
            pha
            phx
            lda         #E_INVAL
            jsr         y_errcode
            plx
            pla
            bra         y_error1
y_error:
            stz         y_code
            stz         y_code + 1
y_error1:
            pha
            phx
            jsr         y_begin
            plx
            pla
            jsr         out_text
            jmp         y_end

; ex = the error "'fn' message" (the built-in's name, then the message at .A, .X); its code :inval
y_fnerr:
            pha
            phx
            lda         #E_INVAL
            jsr         y_errcode
            jsr         y_begin
            lda         #$27
            jsr         out_byte
            ldx         bi_n
            FARN        2, bi_name
            plx
            pla
            jsr         out_text
            jmp         y_end

; y_code = the atom of the system's error .A (its name, lower case: :noent ...), or NIL if it has none
y_errcode:
            stz         y_code
            stz         y_code + 1
            sta         y_c
            LDAX        err_names
            sta         hq
            stx         hq + 1
@entry:
            lda         (hq)                                ; (Its code: 0, the table's end)
            beq         @rts
            cmp         y_c
            beq         @found
            ldy         #0                                  ; (Past its name)
:
            iny
            lda         (hq),y
            bne         :-
            tya
            sec
            adc         hq
            sta         hq
            bcc         @entry
            inc         hq + 1
            bra         @entry
@found:
            inc         hq                                  ; (Its name: the atom's)
            bne         :+
            inc         hq + 1
:
            ldy         #0
:
            lda         (hq),y
            beq         :+
            iny
            bne         :-
:
            sty         hn
            stz         hn + 1
            lda         #PK_ATOM
            jsr         intern
            bcc         :+
            jmp         y_nomem
:
            MOVW        y_code, hv
@rts:
            rts

; ex = the system's error .A about y_name ("name: text"; NIL: the text alone), its code an atom; C = 0
y_syserr:
            pha
            jsr         y_errcode
            jsr         y_begin
            lda         y_name
            ora         y_name + 1
            beq         :+
            lda         y_name
            ldx         y_name + 1
            ldy         #1
            FARN        2, print_val
            lda         #':'
            jsr         out_byte
            lda         #' '
            jsr         out_byte
:
            pla
            tax
            LDR         r0, y_buf
            txa
            jsr         ERRSTR
            LDAX        y_buf
            jsr         out_text
            jmp         y_end

; ex = the system's error .A about the argument 0 (y_syserr0), or y_path (y_syserrp, a path made a string first)
y_syserr0:
            pha
            jsr         y_arg0
            sta         y_name
            stx         y_name + 1
            pla
            jmp         y_syserr
y_syserrp:
            pha
            LDHQ        y_path, 0                           ; (The path, a string)
            ldy         #0
:
            lda         y_path,y
            beq         :+
            iny
            bne         :-
:
            sty         hn
            jsr         string_make
            bcc         :+
            jmp         y_nomem
:
            MOVW        y_name, hv
            lda         hv
            ldx         hv + 1
            jsr         y_rpush
            pla
            jsr         y_syserr
            dec         rsp
            rts

.bss
y_name:     .res        2                                   ; (y_syserr's: what the error is about)

.segment "CODE5"

; ---- Paths

; The argument at .Y a path (a string; an atom's or a symbol's name): in y_path (.X = 0) or y_path2 (.X = 1), a 0
; after it.  OUT: C = 1 and ex = the error ("'fn' expects a path", :inval; "path: name too long") if it isn't one
y_patharg:
            stx         y_slot
            jsr         y_argy
            sta         y_v
            stx         y_v + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         @string
            cmp         #PK_ATOM
            beq         @name
            cmp         #PK_SYMBOL
            beq         @name
            LDAX        s_ypath
            jsr         y_fnerr
            sec
            rts
@name:
            lda         y_v                                 ; (An atom's or a symbol's name: a blob)
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            jsr         blob_data
            bra         @bytes
@string:
            lda         y_v                                 ; (A string: its length, its blob)
            ldx         y_v + 1
            jsr         deref
            lda         (hp)
            sta         y_n
            ldy         #1
            lda         (hp),y
            sta         y_n + 1
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            MOVW        hn, y_n
@bytes:
            lda         hn + 1                              ; (hp its bytes, hn how many: PATH_MAX at most)
            bne         @long
            lda         hn
            cmp         #PATH_MAX + 1
            bcs         @long
            tay
            ldx         y_slot
            beq         @one
            lda         #0
            sta         y_path2,y
:
            dey
            cpy         #$FF
            beq         @done
            lda         (hp),y
            sta         y_path2,y
            bra         :-
@one:
            lda         #0
            sta         y_path,y
:
            dey
            cpy         #$FF
            beq         @done
            lda         (hp),y
            sta         y_path,y
            bra         :-
@done:
            clc
            rts
@long:
            MOVW        y_name, y_v
            lda         #E_NAMETOOLONG
            jsr         y_syserr
            sec
            rts

; ---- Files' bytes

; The rest of fd y_fd read, whole: ex a string of it (or the error "Too long: ...").  OUT: C = 1 if there's no room;
; .A = the system's error (C = 0, ex not set) if a read fails, else 0
y_slurp:
            jsr         y_begin
@chunk:
            LDR         r0, y_buf
            LDR         r1, 256
            lda         y_fd
            jsr         READ
            bcs         @fail
            sta         y_n
            stx         y_n + 1
            ora         y_n + 1
            beq         @end
            ldx         #0
:
            lda         y_buf,x
            jsr         out_byte
            inx
            cpx         y_n
            bne         :-
            bra         @chunk
@end:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            MOVW        ex, hv
            lda         #0
            clc
            rts
@fail:
            pha
            lda         y_start
            ldx         y_start + 1
            jsr         cap_drop
            pla
            clc
            rts

; The bytes of the string .A, .X written to fd y_fd (a chunk at a time, through y_buf).  OUT: C = 1 and .A = the
; system's error if a write fails
y_spill:
            sta         y_v
            stx         y_v + 1
            jsr         deref
            lda         (hp)                                ; (Its length: y_n)
            sta         y_n
            ldy         #1
            lda         (hp),y
            sta         y_n + 1
            stz         y_i                                 ; (Its byte y_i)
            stz         y_i + 1
@chunk:
            lda         y_n
            ora         y_n + 1
            beq         @done
            lda         y_v                                 ; (Its bytes: found again each time, as WRITE may
            ldx         y_v + 1                             ;   have changed the window)
            ldy         #1
            jsr         cell_get
            jsr         blob_data
            clc
            lda         hp
            adc         y_i
            sta         hp
            lda         hp + 1
            adc         y_i + 1
            sta         hp + 1
            ldx         #0                                  ; (256 at most, to y_buf)
@byte:
            lda         y_n
            bne         :+
            lda         y_n + 1
            beq         @write
:
            txa
            tay
            lda         (hp),y
            sta         y_buf,x
            inx
            lda         y_n
            bne         :+
            dec         y_n + 1
:
            dec         y_n
            cpx         #0
            bne         @byte
@write:
            stx         r1
            stz         r1 + 1
            txa
            bne         :+
            inc         r1 + 1                              ; (256)
:
            clc
            lda         y_i
            adc         r1
            sta         y_i
            lda         y_i + 1
            adc         r1 + 1
            sta         y_i + 1
            LDR         r0, y_buf
            lda         y_fd
            jsr         WRITE
            bcc         @chunk
            rts
@done:
            clc
            rts

; The file y_path opened, .A = the mode (O_*).  OUT: C = 0, y_fd its fd; C = 1, .A = the system's error
y_open:
            pha
            LDR         r0, y_path
            pla
            jsr         OPEN
            bcs         @rts
            sta         y_fd
@rts:
            rts
; The file y_path opened to write, emptied if it's there, made if it isn't (y_create); or to write at its end
; (y_append).  OUT: as y_open's
y_append_to:
            lda         #O_WRITE
            jsr         y_open
            bcs         y_made
            stz         r0                                  ; (Its end)
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #2
            lda         y_fd
            jsr         SEEK
            bcc         y_madeok
            pha
            lda         y_fd
            jsr         CLOSE
            pla
            sec
            rts
y_create:
            lda         #O_WRITE | O_TRUNC
            jsr         y_open
            bcc         y_madeok
y_made:
            cmp         #E_NOENT                            ; (Not there: made)
            sec
            bne         y_madeok
            LDR         r0, y_path
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            bcs         y_madeok
            sta         y_fd
y_madeok:
            rts

; y_path's stat record, in y_buf.  OUT: C = 1, .A = the system's error, if there's none
y_stat:
            LDR         r0, y_path
            LDR         r1, y_buf
            jmp         STAT

; Is y_buf's stat record a directory's?  OUT: Z = 0 if it is
y_isdir:
            lda         y_buf + SR_MODE + 1
            and         #DM_DIR
            rts

.segment "RODATA5"
err_names:  ERR_NAMES
s_ypath:    .byte       "' expects a path", 0

.segment "CODE5"
