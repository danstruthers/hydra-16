; ****************************************************************************
; hysys.inc - hylang's system calls (docs/hylang.md, "Made for the Hydra", 4; the plan's phase 10), in its sixth
; bank: (sys name arg...) makes any call a program makes (spec/api.def's, as forth's sys- words), its arguments and
; its value as the call's hl: line has them (obj/gen/hylsys.inc: tools/apigen.js makes the calls' records from
; them).  Each is a function too, sys- and the call's name: (sys-open "x" 0) is (sys :open "x" 0).  A sys- name is
; bound as it's first looked up, unbound (eval.inc's lookup: sys_lazy), to sys partially applied to the name's atom:
; 58 functions, and no more built-ins than sys.
;   What a call reads and fills (its strings, a list's, a buffer, a stat record) is put in the reader's scratch, in
; its last SYS_ROOM bytes: past the number registers, and idle while a built-in runs (the reader's strings and
; names, and a number's digits as it's written, aren't made meanwhile).  A failure is the system's error, about the
; call's first string ("x: not found", y_syserr's).

SYS_ROOM        = 1536          ; The room for a call's strings and buffer (TASKREAD's: ENV_SIZE)
SYS_REC         = 96            ; (A call's record, copied: hylsys.inc's, at most)
sy_area         = scratch + SCRATCH_SIZE - SYS_ROOM
sy_end          = scratch + SCRATCH_SIZE
.assert     sy_area >= ((scratch + 255) & $FF00) + NREGS * 256, error, "sys's room runs into the number registers"

.segment "RODATA6"
.include "hylsys.inc"

.bss
sy_rec:     .res        SYS_REC                             ; The call's record (hylsys.inc's), copied ...
sy_pa:      .res        1                                   ;   its address's place in it ...
sy_p:       .res        1                                   ;   the next input's or output's
sy_k:       .res        1                                   ; (An input's or an output's kind ...
sy_r:       .res        1                                   ;   and register ...
sy_i:       .res        1                                   ;   and how many to go)
sy_one:     .res        1                                   ; (Its outputs: one, its value; more, a list)
sy_given:   .res        1                                   ; The arguments given (after the name) ...
sy_ai:      .res        1                                   ;   and the next one's place at argp
sy_next:    .res        2                                   ; The room's next byte ...
sy_buf:     .res        2                                   ;   the buffer (or the stat record) ...
sy_bsz:     .res        2                                   ;   and its size
sy_call:    .res        2                                   ; The call's address
sy_ia:      .res        1                                   ; .A, .X and .Y for it ...
sy_ix:      .res        1
sy_iy:      .res        1
sy_a:       .res        1                                   ;   and from it
sy_x:       .res        1
sy_y:       .res        1
sy_at:      .res        2                                   ; (A string's place in the room)
sy_l:       .res        2                                   ; (A list walked ...
sy_e:       .res        2                                   ;   an entry, a value ...
sy_t:       .res        2                                   ;   a length ...
sy_w:       .res        2                                   ;   a word ...
sy_c:       .res        1                                   ;   a count)

.segment "CODE6"

; (sys name arg...): the call named (an atom, a symbol or a string: open, sleep-until ...) made, its arguments as
; its hl: line has them; its value, or the error
bi_sys:
            tsx                                             ; (The abort point, y_enter's: the stack as at the entry,
            stx         y_sp                                ;   the root stack, the captures)
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
            jsr         sy_arg0                             ; (Its name)
            jsr         sy_name
            bcs         @none
            jsr         sy_find
            bcc         sy_run
@none:
            lda         #E_INVAL                            ; ("'sys' has no call :x")
            FARN        5, y_errcode
            FARN        5, y_begin
            LDAX        s_snocall
            jsr         out_text
            jsr         sy_arg0
            ldy         #0
            FARN        2, print_val
            FARN        5, y_end
            rts

; .A, .X = the argument 0: the call's name
sy_arg0:
            lda         (argp)
            pha
            ldy         #1
            lda         (argp),y
            tax
            pla
            rts

; The call in sy_rec made with the arguments at argp after its name (cn - 1 of them); ex its value, or the error
sy_run:
            lda         cn                                  ; (As many as it takes?)
            dec         a
            sta         sy_given
            ldy         sy_pa
            iny
            iny
            cmp         sy_rec,y
            bcs         :+
            lda         sy_rec,y                            ; (Too few)
            sta         sy_c
            LDAX        s_satleast
            jmp         sy_arity
:
            iny
            cmp         sy_rec,y
            beq         :+
            bcc         :+
            lda         sy_rec,y                            ; (Too many)
            sta         sy_c
            LDAX        s_satmost
            jmp         sy_arity
:
            jsr         out_flush                           ; (What's out first: what the call writes after it)
            ldx         #31                                 ; (The call's registers: 0 unless they're given)
:
            stz         r0,x
            dex
            bpl         :-
            stz         sy_ia
            stz         sy_ix
            stz         sy_iy
            stz         y_name
            stz         y_name + 1
            stz         sy_bsz
            stz         sy_bsz + 1
            LDAX        sy_area
            sta         sy_next
            stx         sy_next + 1
            lda         #2
            sta         sy_ai
            ldy         sy_pa                               ; (Its address)
            lda         sy_rec,y
            sta         sy_call
            lda         sy_rec + 1,y
            sta         sy_call + 1
            tya                                             ; (Its inputs: past the arguments' counts)
            clc
            adc         #4
            tay
            lda         sy_rec,y
            sta         sy_i
            iny
            sty         sy_p
@in:
            lda         sy_i
            beq         @call
            jsr         sy_kind
            jsr         @input
            dec         sy_i
            bra         @in
@input:
            jmp         (sk_tab - 2,x)
@call:
            lda         sy_ia
            ldx         sy_ix
            ldy         sy_iy
            jsr         sy_go
            sta         sy_a
            stx         sy_x
            sty         sy_y
            bcc         :+
            FARN        5, y_syserr                         ; (Its error, about its first string)
            rts
:
            ldy         sy_p                                ; (What it gives)
            lda         sy_rec,y
            sta         sy_i
            sta         sy_one
            iny
            sty         sy_p
            stz         cf
            stz         cf + 1
            stz         cr
            stz         cr + 1
            stz         ex
            stz         ex + 1
@out:
            lda         sy_i
            beq         @done
            jsr         sy_kind
            jsr         @output
            lda         sy_one
            cmp         #1
            bne         :+
            MOVW        ex, hv
            clc
            rts
:
            lda         hv
            ldx         hv + 1
            FARN        5, y_append
            dec         sy_i
            bra         @out
@output:
            jmp         (so_tab - 2,x)
@done:
            lda         sy_one
            beq         :+
            MOVW        ex, cf
:
            clc
            rts

sy_go:
            jmp         (sy_call)

; The next input's or output's kind and register (sy_k, sy_r), sy_p past them.  OUT: .X = the kind times 2
sy_kind:
            ldy         sy_p
            lda         sy_rec,y
            sta         sy_k
            iny
            lda         sy_rec,y
            sta         sy_r
            iny
            sty         sy_p
            lda         sy_k
            asl
            tax
            rts

; ---- Errors

; ex = the error "'sys-x' text" (the text at .A, .X: in this bank), its code :inval; back from the entry
sy_fail:
            jsr         sy_fnerr
            ldx         y_sp
            txs
            lda         y_rsp
            sta         rsp
            clc
            rts

; ex = the error "'sys-x' text" (the text at .A, .X: in this bank), its code :inval; C = 0
sy_fnerr:
            pha
            phx
            jsr         sy_begin
            plx
            pla
            jsr         out_text
            FARN        5, y_end
            rts

; An error's message begun, its code :inval: "'sys-x'"
sy_begin:
            lda         #E_INVAL
            FARN        5, y_errcode
            FARN        5, y_begin
            LDAX        s_squote
            jsr         out_text
            LDAX        sy_rec
            jsr         out_text
            lda         #$27
            jmp         out_byte

; ex = the error "'sys-x' takes [at least, at most] n argument(s), not g" (.A, .X: at least or at most; sy_c = n);
; C = 0
sy_arity:
            sta         sy_w
            stx         sy_w + 1
            ldy         sy_pa                               ; (Needs as many as it takes: neither)
            iny
            iny
            lda         sy_rec,y
            iny
            cmp         sy_rec,y
            bne         :+
            LDAX        s_snoword
            sta         sy_w
            stx         sy_w + 1
:
            jsr         sy_begin
            LDAX        s_stakes
            jsr         out_text
            lda         sy_w
            ldx         sy_w + 1
            jsr         out_text
            lda         sy_c
            ldx         #0
            jsr         out_dec
            LDAX        s_sarg
            jsr         out_text
            lda         sy_c
            cmp         #1
            beq         :+
            lda         #'s'
            jsr         out_byte
:
            LDAX        s_snot
            jsr         out_text
            lda         sy_given
            ldx         #0
            jsr         out_dec
            FARN        5, y_end
            rts

; ---- The calls' records

; hp, hn: a call's name?  OUT: C = 0, its record in sy_rec (sy_pa: its address's place); C = 1 if there's none
sy_find:
            lda         hn + 1
            bne         @none
            stz         sy_c
@call:
            lda         sy_c
            cmp         #SYS_CALLS
            bcs         @none
            asl
            tax
            lda         sys_calls,x
            sta         hq
            lda         sys_calls + 1,x
            sta         hq + 1
            ldy         #0
:
            cpy         hn
            beq         @end
            lda         (hq),y
            beq         @next
            cmp         (hp),y
            bne         @next
            iny
            bra         :-
@end:
            lda         (hq),y
            beq         @found
@next:
            inc         sy_c
            bra         @call
@found:
            iny
            sty         sy_pa
            ldy         #SYS_REC - 1
:
            lda         (hq),y
            sta         sy_rec,y
            dey
            bpl         :-
            clc
            rts
@none:
            sec
            rts

; ex, a symbol looked up, unbound (eval.inc's lookup): sys-x, a call's function?  It's bound then (globally: sys
; partially applied to the atom :x).  OUT: C = 0, ex its value; C = 1 if it isn't one (ex as it was; .A = 0), or
; there's no room (.A = E_NOMEM)
sys_lazy:
            lda         ex                                  ; (Its name: sys- and a call's)
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            jsr         blob_data
            lda         hn + 1
            beq         @lb709
            jmp         @no
@lb709:
            lda         hn
            cmp         #5
            bcs         @lb708
            jmp         @no
@lb708:
            ldy         #3
:
            lda         (hp),y
            cmp         s_sysp,y
            bne         @no
            dey
            bpl         :-
            lda         hp
            clc
            adc         #4
            sta         hp
            bcc         :+
            inc         hp + 1
:
            lda         hn
            sec
            sbc         #4
            sta         hn
            jsr         sy_find
            bcs         @no
            ldy         sy_pa                               ; (The atom of its name: sy_rec's)
            dey
            sty         hn
            stz         hn + 1
            LDAX        sy_rec
            sta         hq
            stx         hq + 1
            lda         #PK_ATOM
            jsr         intern
            bcs         @nomem
            MOVW        et, hv                              ; ((:x))
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         make2
            bcs         @nomem
            MOVW        eu, hv                              ; (sys and it: a partially applied built-in)
            lda         #<(BUILTIN0 + 2 * BIN_SYS)
            sta         et
            lda         #>(BUILTIN0 + 2 * BIN_SYS)
            sta         et + 1
            lda         #PK_PARTIAL
            jsr         make2
            bcs         @nomem
            lda         ex                                  ; (Its global value)
            ldx         ex + 1
            jsr         deref
            ldy         #2
            lda         hv
            sta         (hp),y
            iny
            lda         hv + 1
            sta         (hp),y
            MOVW        ex, hv
            clc
            rts
@no:
            lda         #0
            sec
            rts
@nomem:
            lda         #E_NOMEM
            sec
            rts

; ---- Arguments

; .A, .X = the next argument.  OUT: C = 1 if it's left out (.A, .X = NIL)
sy_arg:
            lda         sy_ai
            lsr
            dec         a
            cmp         sy_given
            bcs         @out
            ldy         sy_ai
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            iny
            sty         sy_ai
            clc
            rts
@out:
            inc         sy_ai
            inc         sy_ai
            lda         #0
            tax
            sec
            rts

; .A, .X a value: hp, hn its bytes, a string's or an atom's or a symbol's name.  OUT: C = 1 if it's none of those
sy_name:
            sta         sy_w
            stx         sy_w + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         @string
            cmp         #PK_ATOM
            beq         @name
            cmp         #PK_SYMBOL
            beq         @name
            sec
            rts
@name:
            lda         sy_w
            ldx         sy_w + 1
            ldy         #0
            jsr         cell_get
            jsr         blob_data
            clc
            rts
@string:
            lda         sy_w                                ; (Its length, its blob)
            ldx         sy_w + 1
            jsr         deref
            lda         (hp)
            sta         sy_t
            ldy         #1
            lda         (hp),y
            sta         sy_t + 1
            ora         sy_t
            beq         @empty
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
@empty:
            MOVW        hn, sy_t
            clc
            rts

; .A, .X a character: .A, .X = its code (a fixnum); anything else as it is
sy_code:
            pha
            lsr
            pla
            bcs         @rts
            cpx         #>CHAR0
            bcc         @rts
            cpx         #>CHAR0 + 2
            bcs         @rts
            pha
            txa
            lsr
            pla
            ror
            ldx         #0
            jmp         fix_ax
@rts:
            rts

; .A, .X a fixnum 0-255: .A it.  OUT: C = 1 if it isn't one
sy_byte:
            cpx         #2
            bcs         @no
            pha
            lsr
            pla
            bcc         @no
            pha
            txa
            lsr
            pla
            ror
            clc
            rts
@no:
            sec
            rts

; ---- The call's registers

; The register sy_r (rN) = .A, .X
sy_preg:
            sta         sy_w
            stx         sy_w + 1
            lda         sy_r
            and         #$0F
            asl
            tax
            lda         sy_w
            sta         r0,x
            lda         sy_w + 1
            sta         r0 + 1,x
            rts

; The register sy_r = n_w32 (as much of it as the register holds)
sy_put:
            lda         sy_r
            cmp         #$20
            bcc         @r
            cmp         #$21
            beq         @x
            cmp         #$22
            beq         @y
            pha
            lda         n_w32
            sta         sy_ia
            pla
            cmp         #$23
            bne         @rts
            lda         n_w32 + 1
            sta         sy_ix
            rts
@x:
            lda         n_w32
            sta         sy_ix
            rts
@y:
            lda         n_w32
            sta         sy_iy
            rts
@r:
            and         #$0F
            asl
            tax
            lda         n_w32
            sta         r0,x
            lda         n_w32 + 1
            sta         r0 + 1,x
            lda         sy_r
            and         #$10
            beq         @rts
            lda         n_w32 + 2
            sta         r0 + 2,x
            lda         n_w32 + 3
            sta         r0 + 3,x
@rts:
            rts

; n_w32 = the register sy_r as the call left it
sy_get:
            stz         n_w32 + 1
            stz         n_w32 + 2
            stz         n_w32 + 3
            lda         sy_r
            cmp         #$20
            bcc         @r
            cmp         #$21
            beq         @x
            cmp         #$22
            beq         @y
            pha
            lda         sy_a
            sta         n_w32
            pla
            cmp         #$23
            bne         @rts
            lda         sy_x
            sta         n_w32 + 1
            rts
@x:
            lda         sy_x
            sta         n_w32
            rts
@y:
            lda         sy_y
            sta         n_w32
            rts
@r:
            and         #$0F
            asl
            tax
            lda         r0,x
            sta         n_w32
            lda         r0 + 1,x
            sta         n_w32 + 1
            lda         sy_r
            and         #$10
            beq         @rts
            lda         r0 + 2,x
            sta         n_w32 + 2
            lda         r0 + 3,x
            sta         n_w32 + 3
@rts:
            rts

; ---- The room

; hn bytes of the room taken.  OUT: hq = their place (sy_next past them); none left, the error
sy_take:
            lda         sy_next
            sta         hq
            clc
            adc         hn
            sta         sy_w
            lda         sy_next + 1
            sta         hq + 1
            adc         hn + 1
            sta         sy_w + 1
            bcs         sy_big
            lda         #<sy_end
            cmp         sy_w
            lda         #>sy_end
            sbc         sy_w + 1
            bcc         sy_big
            MOVW        sy_next, sy_w
            rts
sy_big:
            LDAX        s_sroom
            jmp         sy_fail

; hn bytes at hq = .A (hq moved past them)
sy_fill:
            ldy         hn
            bne         :+
            ldy         hn + 1
            beq         @done
            dec         hn + 1
:
            dec         hn
            sta         (hq)
            inc         hq
            bne         sy_fill
            inc         hq + 1
            bra         sy_fill
@done:
            rts

; hn bytes from hp to hq (both moved past them)
sy_copy:
            lda         hn
            ora         hn + 1
            beq         @done
            lda         (hp)
            sta         (hq)
            inc         hp
            bne         :+
            inc         hp + 1
:
            inc         hq
            bne         :+
            inc         hq + 1
:
            lda         hn
            bne         :+
            dec         hn + 1
:
            dec         hn
            bra         sy_copy
@done:
            rts

; hp, hn's bytes and a 0 in the room: sy_at their place (sy_str: the register sy_r too)
sy_str:
            jsr         sy_strb
            lda         sy_at
            ldx         sy_at + 1
            jmp         sy_preg
sy_strb:
            MOVW        sy_t, hn
            inc         hn
            bne         :+
            inc         hn + 1
            beq         sy_big
:
            jsr         sy_take
            MOVW        sy_at, hq
            MOVW        hn, sy_t
            jsr         sy_copy
            lda         #0
            sta         (hq)
            rts

; ---- Inputs: each kind's (sy_r its register; sy_p past the kind and register, at a parameter)

sk_tab:     .word       sk_n, sk_nq, sk_i, sk_s, sk_sq, sk_args, sk_map, sk_bytes, sk_bufn, sk_buf, sk_count
            .word       sk_size, sk_io, sk_stat

; n?: a whole number, or NIL or left out (0)
sk_nq:
            jsr         sy_arg
            bcs         sk_rts
            cmp         #0
            bne         sk_num
            cpx         #0
            bne         sk_num
sk_rts:
            rts

; n: a whole number (or a character, its code), the register's size
sk_n:
            jsr         sy_arg
sk_num:
            jsr         sy_code
            FARN        3, n_to_u32
            bcs         @not
            ldx         #1                                  ; (Its bytes: the register's at most)
            lda         sy_r
            cmp         #$20
            bcs         :+
            inx
            cmp         #$10
            bcc         :+
            ldx         #4
:
            cmp         #$23
            bne         :+
            ldx         #2
:
            stx         sy_c
:
            cpx         #4
            beq         @put
            lda         n_w32,x
            bne         @range
            inx
            bra         :-
@put:
            jmp         sy_put
@range:
            LDAX        s_sbyte
            ldy         sy_c
            cpy         #1
            beq         :+
            LDAX        s_sword
:
            jmp         sy_fail
@not:
            LDAX        s_snum
            jmp         sy_fail

; i: an integer, -2^31 to 2^32 - 1 (32 bits)
sk_i:
            jsr         sy_arg
            jsr         sy_code
            FARN        3, n_to_i32
            bcs         :+
            jmp         sy_put
:
            LDAX        s_sint
            jmp         sy_fail

; s?: a string, or NIL or left out (0)
sk_sq:
            jsr         sy_arg
            bcc         @lb707
            jmp         sk_rts
@lb707:
            cmp         #0
            bne         sk_str
            cpx         #0
            bne         sk_str
            rts

; s: a string (an atom's or a symbol's name), its bytes and a 0; the first, what an error is about
sk_s:
            jsr         sy_arg
sk_str:
            pha
            phx
            jsr         sy_name
            plx
            pla
            bcs         sk_notstr
            ldy         y_name + 1
            bne         :+
            ldy         y_name
            bne         :+
            sta         y_name
            stx         y_name + 1
:
            jmp         sy_str
sk_notstr:
            LDAX        s_sstr
            jmp         sy_fail

; args?: a list of strings, each with a 0, then an empty one; or NIL, or left out (0)
sk_args:
            jsr         sy_arg
            bcc         @lb706
            jmp         sk_rts
@lb706:
            cmp         #0
            bne         :+
            cpx         #0
            bne         @lb705
            jmp         sk_rts
@lb705:
:
            jsr         q_conses
            bcs         @not
            sta         sy_l
            stx         sy_l + 1
            lda         sy_next
            ldx         sy_next + 1
            jsr         sy_preg
@item:
            lda         sy_l
            ora         sy_l + 1
            beq         @end
            lda         sy_l
            ldx         sy_l + 1
            ldy         #0
            jsr         cell_get
            jsr         sy_name
            bcs         @not
            jsr         sy_strb
            lda         sy_l
            ldx         sy_l + 1
            ldy         #1
            jsr         cell_get
            sta         sy_l
            stx         sy_l + 1
            bra         @item
@end:
            lda         #1                                  ; (The empty one)
            sta         hn
            stz         hn + 1
            jsr         sy_take
            lda         #0
            sta         (hq)
            rts
@not:
            LDAX        s_sargs
            jmp         sy_fail

; map?: a list of bytes after their count; or NIL, or left out (0)
sk_map:
            jsr         sy_arg
            bcc         @lb704
            jmp         sk_rts
@lb704:
            cmp         #0
            bne         :+
            cpx         #0
            bne         @lb703
            jmp         sk_rts
@lb703:
:
            jsr         q_conses
            bcs         @not
            sta         sy_l
            stx         sy_l + 1
            lda         #1                                  ; (Its count)
            sta         hn
            stz         hn + 1
            jsr         sy_take
            MOVW        sy_at, hq
            lda         hq
            ldx         hq + 1
            jsr         sy_preg
            stz         sy_c
@item:
            lda         sy_l
            ora         sy_l + 1
            beq         @end
            lda         sy_l
            ldx         sy_l + 1
            ldy         #0
            jsr         cell_get
            jsr         sy_byte
            bcs         @not
            pha
            lda         #1
            sta         hn
            stz         hn + 1
            jsr         sy_take
            pla
            sta         (hq)
            inc         sy_c
            lda         sy_l
            ldx         sy_l + 1
            ldy         #1
            jsr         cell_get
            sta         sy_l
            stx         sy_l + 1
            bra         @item
@end:
            MOVW        hq, sy_at
            lda         sy_c
            sta         (hq)
            rts
@not:
            LDAX        s_smap
            jmp         sy_fail

; bytes(R): a string's bytes, their count in R (the parameter)
sk_bytes:
            jsr         sy_arg
            jsr         sy_name
            bcc         @lb702
            jmp         sk_notstr
@lb702:
            MOVW        sy_t, hn
            jsr         sy_take
            lda         hq
            ldx         hq + 1
            jsr         sy_preg
            ldy         sy_p                                ; (Its count's register)
            lda         sy_rec,y
            iny
            sty         sy_p
            and         #$0F
            asl
            tax
            lda         sy_t
            sta         r0,x
            lda         sy_t + 1
            sta         r0 + 1,x
            MOVW        hn, sy_t
            jmp         sy_copy

; buf(N): a buffer of N bytes (the parameter), zeros
sk_bufn:
            ldy         sy_p
            lda         sy_rec,y
            sta         hn
            sta         sy_bsz
            iny
            lda         sy_rec,y
            sta         hn + 1
            sta         sy_bsz + 1
            iny
            sty         sy_p
            jsr         sy_take
            MOVW        sy_buf, hq
            lda         hq
            ldx         hq + 1
            jsr         sy_preg
            lda         #0
            jmp         sy_fill

; buf: a buffer of the room that's left (count or size after it: its size)
sk_buf:
            MOVW        sy_buf, sy_next
            sec
            lda         #<sy_end
            sbc         sy_next
            sta         sy_bsz
            lda         #>sy_end
            sbc         sy_next + 1
            sta         sy_bsz + 1
            LDAX        sy_end
            sta         sy_next
            stx         sy_next + 1
            lda         sy_buf
            ldx         sy_buf + 1
            jmp         sy_preg

; count: a whole number, buf's size: the room at most; zeros (what the call doesn't fill)
sk_count:
            jsr         sk_count1
            MOVW        hq, sy_buf
            MOVW        hn, sy_bsz
            lda         #0
            jmp         sy_fill
sk_count1:
            jsr         sy_arg
            jsr         sy_code
            FARN        3, n_to_u32
            bcs         @not
            lda         n_w32 + 2
            ora         n_w32 + 3
            bne         sk_size
            lda         n_w32
            cmp         sy_bsz
            lda         n_w32 + 1
            sbc         sy_bsz + 1
            bcs         sk_size
            MOVW        sy_bsz, n_w32
            jmp         sy_put
@not:
            LDAX        s_snum
            jmp         sy_fail

; size: buf's size
sk_size:
            MOVW        n_w32, sy_bsz
            stz         n_w32 + 2
            stz         n_w32 + 3
            jmp         sy_put

; io(N): a buffer of N bytes, a string's (as many as fit) or zeros
sk_io:
            jsr         sk_bufn
            jsr         sy_arg
            bcs         @rts
            cmp         #0
            bne         :+
            cpx         #0
            beq         @rts
:
            jsr         sy_name
            bcc         @lb701
            jmp         sk_notstr
@lb701:
            lda         hn
            cmp         sy_bsz
            lda         hn + 1
            sbc         sy_bsz + 1
            bcc         :+
            MOVW        hn, sy_bsz
:
            MOVW        hq, sy_buf
            jmp         sy_copy
@rts:
            rts

; stat: a hash (stat's: :name, :mode, :length, :mtime) made a stat record; what isn't in it, all $FF (an empty name):
; left as it is
sk_stat:
            lda         #SR_SIZE
            sta         hn
            stz         hn + 1
            jsr         sy_take
            MOVW        sy_buf, hq
            lda         hq
            ldx         hq + 1
            jsr         sy_preg
            lda         #$FF
            jsr         sy_fill
            MOVW        hq, sy_buf
            lda         #0
            ldy         #SR_NAME
            sta         (hq),y
            jsr         sy_arg
            sta         sy_l
            stx         sy_l + 1
            jsr         kind_of
            cmp         #PK_HASH
            beq         :+
            jmp         @not
:
            lda         sy_l                                ; (Its entries: each a list, key and value; its tags
            ldx         sy_l + 1                            ;   after them)
            ldy         #H_ITEMS
            jsr         cell_get
@item:
            sta         sy_l
            stx         sy_l + 1
            ora         sy_l + 1
            beq         @rts
            lda         sy_l
            ldx         sy_l + 1
            ldy         #0
            jsr         cell_get
            sta         sy_e
            stx         sy_e + 1
            jsr         list_kind
            cmp         #PK_QCONS
            bne         @rts
            lda         sy_e                                ; (Its key)
            ldx         sy_e + 1
            ldy         #0
            jsr         cell_get
            sta         sy_t
            stx         sy_t + 1
            lda         sy_e                                ; (Its value)
            ldx         sy_e + 1
            ldy         #1
            jsr         cell_get
            ldy         #0
            jsr         cell_get
            sta         sy_e
            stx         sy_e + 1
            lda         sy_t                                ; (Its key an atom, one of them?)
            ldx         sy_t + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         @next
            lda         sy_t
            jsr         sy_name
            stz         sy_c
@key:
            lda         sy_c
            asl
            tay
            lda         sk_keys,y
            ldx         sk_keys + 1,y
            jsr         sy_is
            bcc         @field
            inc         sy_c
            lda         sy_c
            cmp         #4
            bcc         @key
@next:
            lda         sy_l
            ldx         sy_l + 1
            ldy         #1
            jsr         cell_get
            jmp         @item
@rts:
            rts
@field:
            lda         sy_c
            bne         @number
            lda         sy_e                                ; (:name, a string: 31 bytes at most)
            ldx         sy_e + 1
            jsr         kind_of
            cmp         #PK_STRING
            bne         @not
            lda         sy_e
            ldx         sy_e + 1
            jsr         sy_name
            lda         hn + 1
            bne         @not
            lda         hn
            cmp         #32
            bcs         @not
            MOVW        hq, sy_buf
            ldy         hn
            lda         #0
            sta         (hq),y
            jsr         sy_copy
            bra         @next
@number:
            lda         sy_e                                ; (:mode, :length, :mtime: a whole number)
            ldx         sy_e + 1
            FARN        3, n_to_u32
            bcs         @not
            ldx         sy_c
            lda         sk_at - 1,x
            clc
            adc         sy_buf
            sta         hq
            lda         sy_buf + 1
            adc         #0
            sta         hq + 1
            ldy         #3
            cpx         #1
            bne         :+
            lda         n_w32 + 2                           ; (A mode: 16 bits)
            ora         n_w32 + 3
            bne         @not
            ldy         #1
:
            lda         n_w32,y
            sta         (hq),y
            dey
            bpl         :-
            jmp         @next
@not:
            LDAX        s_sstat
            jmp         sy_fail

; Whether hp, hn's bytes are the zero-terminated text at .A, .X (in this bank).  OUT: C = 0 if they are
sy_is:
            sta         hq
            stx         hq + 1
            lda         hn + 1
            bne         @no
            ldy         #0
:
            cpy         hn
            beq         @end
            lda         (hq),y
            beq         @no
            cmp         (hp),y
            bne         @no
            iny
            bra         :-
@end:
            lda         (hq),y
            bne         @no
            clc
            rts
@no:
            sec
            rts

; ---- Outputs: each kind's (sy_r its register): hv its value

so_tab:     .word       so_reg, so_buf, so_z, so_bufreg, so_stat

; REG: a whole number
so_reg:
            jsr         sy_get
            FARN        5, y_u32
            rts

; buf: the buffer, a string
so_buf:
            MOVW        hn, sy_bsz
so_make:
            MOVW        hq, sy_buf
            jsr         string_make
            bcc         :+
            FARN        5, y_nomem
:
            rts

; buf=z: the buffer's text, to its 0
so_z:
            MOVW        hq, sy_buf
            ldx         sy_bsz
            lda         sy_bsz + 1
            beq         :+
            ldx         #255
:
            stx         sy_c
            ldy         #0
:
            cpy         sy_c
            beq         :+
            lda         (hq),y
            beq         :+
            iny
            bra         :-
:
            sty         hn
            stz         hn + 1
            bra         so_make

; buf=REG: the buffer's first REG bytes (its size at most)
so_bufreg:
            jsr         sy_get
            MOVW        hn, n_w32
            lda         hn
            cmp         sy_bsz
            lda         hn + 1
            sbc         sy_bsz + 1
            bcc         so_make
            bra         so_buf

; buf=stat: the stat record, a hash (stat's)
so_stat:
            MOVW        hq, sy_buf
            ldy         #SR_SIZE - 1
:
            lda         (hq),y
            sta         y_buf,y
            dey
            bpl         :-
            lda         cf                                  ; (The list being made, as it is)
            ldx         cf + 1
            FARN        5, y_rpush
            lda         cr
            ldx         cr + 1
            FARN        5, y_rpush
            FARN        5, y_stathash
            jsr         root_pop
            sta         cr
            stx         cr + 1
            jsr         root_pop
            sta         cf
            stx         cf + 1
            rts

.segment "RODATA6"
sk_keys:    .word       s_kname6, s_kmode6, s_klength6, s_kmtime6
sk_at:      .byte       SR_MODE, SR_LENGTH, SR_MTIME
s_kname6:   .byte       "name", 0
s_kmode6:   .byte       "mode", 0
s_klength6: .byte       "length", 0
s_kmtime6:  .byte       "mtime", 0
s_sysp:     .byte       "sys-"
s_snocall:  .byte       "'sys' has no call ", 0
s_squote:   .byte       "'sys-", 0
s_stakes:   .byte       " takes ", 0
s_satleast: .byte       "at least ", 0
s_satmost:  .byte       "at most ", 0
s_snoword:    .byte       0
s_sarg:     .byte       " argument", 0
s_snot:     .byte       ", not ", 0
s_snum:     .byte       " expects a whole number", 0
s_sbyte:    .byte       " expects a whole number below 256", 0
s_sword:    .byte       " expects a whole number below 65536", 0
s_sint:     .byte       " expects an integer, -2^31 to 2^32 - 1", 0
s_sstr:     .byte       " expects a string", 0
s_sargs:    .byte       " expects a list of strings", 0
s_smap:     .byte       " expects a list of bytes", 0
s_sstat:    .byte       " expects a stat hash", 0
s_sroom:    .byte       " expects less: 1536 bytes of strings and buffer at most", 0
