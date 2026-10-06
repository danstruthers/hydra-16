; ****************************************************************************
; hydrabi.inc - hylang's Hydra built-ins (docs/hylang.md, "Made for the Hydra", 2; the plan's phase 9), in its fifth
; bank beside the system library, whose helpers they share (sys.inc): the system (sysinfo, mods, errstr), the tasks
; (ps, task-info, yield, sleep-until), memory (peek, poke and their words, the task's banks, shared segments, free),
; namespaces (bind, mount, unmount, ns; newns is a special form: forms.inc), notes (note, note-group, on-note: the evaluator calls its function,
; eval.inc's ev_note) and raw keys (key, key?).  hold is a special form (forms.inc).  A record the system gives is a
; hash, its keys atoms; a list of them is a list of hashes.

.ifndef U_REGISTER
U_REGISTER      = $FFF1         ; (include/hw.inc's: the shared macro-page, written before $00 for a segment's bank)
.endif

.bss
hd_i:       .res        1                                   ; (A count, an index ...
hd_t:       .res        1                                   ;   a task ...
hd_n:       .res        2                                   ;   bytes to go ...
hd_bank:    .res        1                                   ;   a window's $00 and U, and whether it's a segment's
hd_u:       .res        1
hd_seg:     .res        1
hd_w:       .res        2                                   ;   a word's bits ...
hd_x:       .res        1                                   ;   and a call's .A, .X)
hd_a:       .res        1
hd_info:    .res        TI_SIZE                             ; (TASKINFO's answer)
hd_spec:    .res        1                                   ; (mount's spec: its argument's offset)

.segment "CODE5"

; ---- Helpers

; The argument at .Y a whole number (0 to 2^32 - 1): n_w32.  OUT: C = 1 and ex = the error ("'fn' expects a whole
; number", :inval) if it isn't one
hd_word:
            jsr         y_argy
            FARN        3, n_to_u32
            bcc         @rts
            LDAX        s_hwhole
            jsr         y_fnerr
            sec
@rts:
            rts

; The argument at .Y a whole number below .A (0: 256): .A.  OUT: C = 1, ex the error, if it isn't one
hd_byte:
            sta         hd_x
            jsr         hd_word
            bcs         @rts
            lda         n_w32 + 1
            ora         n_w32 + 2
            ora         n_w32 + 3
            bne         @range
            lda         hd_x
            beq         :+
            lda         n_w32
            cmp         hd_x
            bcs         @range
:
            lda         n_w32
            clc
@rts:
            rts
@range:
            LDAX        s_hrange
            jsr         y_fnerr
            sec
            rts

; The argument at .Y a whole number below 65536 (an address, a word): .A, .X and hq.  OUT: C = 1, ex the error, if
; it isn't one
hd_addr:
            jsr         hd_word
            bcs         @rts
            lda         n_w32 + 2
            ora         n_w32 + 3
            bne         @range
            lda         n_w32
            sta         hq
            ldx         n_w32 + 1
            stx         hq + 1
            clc
@rts:
            rts
@range:
            LDAX        s_hrange
            jsr         y_fnerr
            sec
            rts

; ex = the whole number .A, .X (0-65535); C = 0
hd_u16:
            sta         n_w32
            stx         n_w32 + 1
            stz         n_w32 + 2
            stz         n_w32 + 3
            jmp         y_u32

; hv = the whole number .A (0-255)
hd_fix:
            ldx         #0
            jsr         fix_ax
            sta         hv
            stx         hv + 1
            rts

; hv = the string of the zero-terminated bytes at .A, .X (at most .Y of them: 255 for no limit)
hd_zstr:
            sta         hq
            stx         hq + 1
            sty         hd_x
            ldy         #0
:
            cpy         hd_x
            beq         :+
            lda         (hq),y
            beq         :+
            iny
            bne         :-
:
            sty         hn
            stz         hn + 1
            jsr         string_make
            bcc         :+
            jmp         y_nomem
:
            rts

; A hash begun: the list being made (cf, cr) kept on the root stack, a list of its entries begun (hd_end: ended,
; hv the hash, the list as it was)
hd_begin:
            lda         cf
            ldx         cf + 1
            jsr         y_rpush
            lda         cr
            ldx         cr + 1
            jsr         y_rpush
            stz         cf
            stz         cf + 1
            rts
hd_end:
            FARN        4, h_fromlist
            bcc         :+
            jmp         y_nomem
:
            jsr         root_pop
            sta         cr
            stx         cr + 1
            jsr         root_pop
            sta         cf
            stx         cf + 1
            rts

; The entry {:name n}, the number .A (0-255), its name at hq (hd_nentry: the name at .X, .Y)
hd_nentry:
            stx         hq
            sty         hq + 1
hd_nentry_hq:
            jsr         hd_fix
            lda         hq
            ldx         hq + 1
            jmp         y_entry

; hv = the atom of the .A-th of the zero-terminated names at hq, one after another (none past them: NIL)
hd_nth:
            tax
@name:
            lda         (hq)
            beq         @none
            txa
            beq         @this
            dex
            ldy         #0
:
            lda         (hq),y
            iny
            ora         #0
            bne         :-
            tya
            clc
            adc         hq
            sta         hq
            bcc         @name
            inc         hq + 1
            bra         @name
@this:
            lda         hq
            ldx         hq + 1
            jmp         y_atom
@none:
            stz         hv
            stz         hv + 1
            rts

; hv = a list of the numbers of the bits set in hd_w (0-15), lowest first
hd_bits:
            stz         cf
            stz         cf + 1
            stz         hd_i
@bit:
            lsr         hd_w + 1
            ror         hd_w
            bcc         @next
            lda         hd_i
            jsr         hd_fix
            lda         hv
            ldx         hv + 1
            jsr         y_append
@next:
            inc         hd_i
            lda         hd_i
            cmp         #16
            bcc         @bit
            MOVW        hv, cf
            rts

; ---- The system

; (sysinfo): a hash, :abi (the ABI's version), :ram-modules (the RAM modules installed), :free-tasks (a list)
bi_sysinfo:
            jsr         y_enter
            jsr         SYSINFO
            sta         hd_a
            stx         hd_x
            MOVW        hd_w, r0
            jsr         hd_bits                             ; (The free tasks: kept as the hash is made)
            lda         hv
            ldx         hv + 1
            jsr         y_rpush
            stz         cf
            stz         cf + 1
            lda         hd_a
            ldx         #<s_kabi
            ldy         #>s_kabi
            jsr         hd_nentry
            lda         hd_x
            ldx         #<s_kram
            ldy         #>s_kram
            jsr         hd_nentry
            jsr         root_pop
            sta         hv
            stx         hv + 1
            LDAX        s_kfree
            jsr         y_entry
            FARN        4, h_fromlist
            bcc         :+
            jmp         y_nomem
:
            MOVW        ex, hv
            clc
            rts

; (mods): the paged ROM's modules, a list of hashes: :name, :type (:program, :driver, :library), :bank (its first),
; :banks
bi_mods:
            jsr         y_enter
            stz         cf
            stz         cf + 1
            stz         hd_i
@mod:
            LDR         r0, y_buf
            lda         hd_i
            jsr         MODINFO
            bcs         @done
            jsr         hd_begin
            LDAX        y_buf + ME_NAME
            ldy         #12
            jsr         hd_zstr
            LDAX        s_kname
            jsr         y_entry
            LDHQ        s_types, 0                          ; (Its type: an atom)
            lda         y_buf + ME_TYPE
            dec         a
            cmp         #3
            bcc         :+
            lda         #3
:
            jsr         hd_nth
            LDAX        s_ktype
            jsr         y_entry
            lda         y_buf + ME_BANK
            ldx         #<s_kbank
            ldy         #>s_kbank
            jsr         hd_nentry
            lda         y_buf + ME_BANKS
            ldx         #<s_kbanks
            ldy         #>s_kbanks
            jsr         hd_nentry
            jsr         hd_end
            lda         hv
            ldx         hv + 1
            jsr         y_append
            inc         hd_i
            bne         @mod
@done:
            MOVW        ex, cf
            clc
            rts

; (errstr code): the system's error's text, a string (code an integer, or an atom: :noent ...); "error $NN" for one
; it doesn't know
bi_errstr:
            jsr         y_enter
            jsr         y_arg0
            jsr         kind_of
            cmp         #PK_ATOM
            beq         @atom
            ldy         #0
            lda         #0                                  ; (256: any byte)
            jsr         hd_word
            bcs         @rts
            lda         n_w32 + 1
            ora         n_w32 + 2
            ora         n_w32 + 3
            beq         @lb587
            jmp         @bad
@lb587:
            lda         n_w32
@code:
            tax
            LDR         r0, y_buf
            txa
            jsr         ERRSTR
            LDAX        y_buf
            ldy         #255
            jsr         hd_zstr
            MOVW        ex, hv
@rts:
            clc
            rts
@atom:
            LDAX        err_names                           ; (Its code: the one whose name's atom it is)
            sta         y_v
            stx         y_v + 1
@name:
            MOVW        hb, y_v
            lda         (hb)
            beq         @bad
            sta         hd_x
            lda         y_v                                 ; (The name, after the code)
            clc
            adc         #1
            sta         y_w
            lda         y_v + 1
            adc         #0
            sta         y_w + 1
            lda         y_w
            ldx         y_w + 1
            jsr         y_atom
            jsr         y_arg0
            cmp         hv
            bne         @skip
            cpx         hv + 1
            bne         @skip
            lda         hd_x
            bra         @code
@skip:
            MOVW        hb, y_w
            ldy         #0
:
            lda         (hb),y
            iny
            ora         #0
            bne         :-
            tya
            clc
            adc         y_w
            sta         y_v
            lda         y_w + 1
            adc         #0
            sta         y_v + 1
            bra         @name
@bad:
            LDAX        s_hcode
            jsr         y_fnerr
            clc
            rts

; ---- Tasks

; (ps): the tasks there are, a list of hashes (task-info's)
bi_ps:
            jsr         y_enter
            stz         cf
            stz         cf + 1
            stz         hd_t
@task:
            jsr         hd_task
            bcs         @next
            lda         hv
            ldx         hv + 1
            jsr         y_append
@next:
            inc         hd_t
            lda         hd_t
            cmp         #16
            bcc         @task
            MOVW        ex, cf
            clc
            rts

; (task-info task): the task's hash: :task, :name, :state (:ready, :wait, :call, :idle, :new, :sleep, :blocked,
; :event), :parent (NIL: none), :cpu (ticks), :group (its notes'), :args (a list of strings)
bi_task_info:
            jsr         y_enter
            ldy         #0
            lda         #16
            jsr         hd_byte
            bcs         @rts
            sta         hd_t
            jsr         hd_task
            bcc         :+
            lda         #E_SRCH
            jmp         y_syserr0
:
            MOVW        ex, hv
@rts:
            clc
            rts

; hv = task hd_t's hash.  OUT: C = 1 if there's no such task
hd_task:
            LDR         r0, hd_info
            lda         hd_t
            jsr         TASKINFO
            bcs         @rts
            lda         hd_info + TI_STATE
            bne         :+
            sec
@rts:
            rts
:
            jsr         hd_begin
            lda         hd_t
            ldx         #<s_ktask
            ldy         #>s_ktask
            jsr         hd_nentry
            LDAX        hd_info + TI_NAME
            ldy         #16
            jsr         hd_zstr
            LDAX        s_kname
            jsr         y_entry
            LDHQ        s_states, 0                         ; (Its state: an atom)
            lda         hd_info + TI_STATE
            dec         a
            jsr         hd_nth
            LDAX        s_kstate
            jsr         y_entry
            stz         hv                                  ; (Its parent: NIL for none)
            stz         hv + 1
            lda         hd_info + TI_PARENT
            cmp         #$FF
            beq         :+
            jsr         hd_fix
:
            LDAX        s_kparent
            jsr         y_entry
            ldx         #2                                  ; (Its time: ticks, 3 bytes)
:
            lda         hd_info + TI_CPU,x
            sta         n_w32,x
            dex
            bpl         :-
            stz         n_w32 + 3
            jsr         y_u32
            LDAX        s_kcpu
            jsr         y_entry
            lda         hd_info + TI_GROUP
            ldx         #<s_kgroup
            ldy         #>s_kgroup
            jsr         hd_nentry
            lda         cf                                  ; (Its arguments: a list of strings, made with the
            ldx         cf + 1                              ;   hash's list kept)
            jsr         y_rpush
            lda         cr
            ldx         cr + 1
            jsr         y_rpush
            stz         cf
            stz         cf + 1
            LDR         r0, y_buf
            lda         hd_t
            ldx         #TR_ARGS
            jsr         TASKREAD
            bcs         @args
            LDAX        y_buf
            sta         y_v
            stx         y_v + 1
@arg:
            MOVW        hb, y_v
            lda         (hb)
            beq         @args
            lda         y_v
            ldx         y_v + 1
            ldy         #255
            jsr         hd_zstr
            lda         hv
            ldx         hv + 1
            jsr         y_append
            MOVW        hb, y_v                             ; (The next: past this one's 0)
            ldy         #0
:
            lda         (hb),y
            iny
            ora         #0
            bne         :-
            tya
            clc
            adc         y_v
            sta         y_v
            bcc         @arg
            inc         y_v + 1
            bra         @arg
@args:
            MOVW        hv, cf
            jsr         root_pop
            sta         cr
            stx         cr + 1
            jsr         root_pop
            sta         cf
            stx         cf + 1
            LDAX        s_kargs
            jsr         y_entry
            jsr         hd_end
            clc
            rts

; (yield): the CPU given to the next task ready; NIL
bi_yield:
            jsr         YIELD
            jmp         y_nil

; (sleep-until tick): asleep till the tick count is tick (32767 ticks ahead at most); NIL (a note ends it early:
; the error :intr)
bi_sleep_until:
            jsr         y_enter
            ldy         #0
            jsr         hd_addr
            bcs         @rts
            cpx         #$80
            bcc         :+
            LDAX        s_hrange
            jsr         y_fnerr
@rts:
            clc
            rts
:
            jsr         SLEEP_UNTIL
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_nil

; ---- Memory

; (peek addr): the byte at addr (the task's own 64K, as it is as hylang runs: its fifth bank at $A000)
bi_peek:
            jsr         y_enter
            ldy         #0
            jsr         hd_addr
            bcs         hd_rts
            lda         (hq)
            ldx         #0
            jmp         y_fix
hd_rts:
            clc
            rts

; (peek-word addr): the word at addr (low byte first)
bi_peek_word:
            jsr         y_enter
            ldy         #0
            jsr         hd_addr
            bcs         hd_rts
            ldy         #1
            lda         (hq),y
            tax
            lda         (hq)
            jmp         hd_u16

; (poke addr byte): byte at addr; NIL
bi_poke:
            jsr         y_enter
            ldy         #2
            lda         #0
            jsr         hd_word
            bcs         hd_rts
            lda         n_w32 + 1
            ora         n_w32 + 2
            ora         n_w32 + 3
            beq         :+
            LDAX        s_hrange
            jsr         y_fnerr
            clc
            rts
:
            lda         n_w32
            pha
            ldy         #0
            jsr         hd_addr
            pla
            bcs         hd_rts
            sta         (hq)
            jmp         y_nil

; (poke-word addr n): n at addr, low byte first (0-65535); NIL
bi_poke_word:
            jsr         y_enter
            ldy         #2
            jsr         hd_addr
            bcs         hd_rts
            MOVW        hd_w, hq
            ldy         #0
            jsr         hd_addr
            bcs         hd_rts
            lda         hd_w
            sta         (hq)
            ldy         #1
            lda         hd_w + 1
            sta         (hq),y
            jmp         y_nil

; (banks): the RAM banks the task has (16 for each good RAM module)
bi_banks:
            jsr         BANKS
            ldx         #0
            jmp         y_fix

; (bank-alloc n): n banks taken (8K each, in a row); the first's number (for $00)
bi_bank_alloc:
            jsr         y_enter
            ldy         #0
            lda         #129
            jsr         hd_byte
            bcc         @lb586
            jmp         hd_rts
@lb586:
            jsr         BANKS_ALLOC
            bcc         :+
            jmp         y_syserr0
:
            ldx         #0
            jmp         y_fix

; (bank-free bank n): n banks given back from bank (bank-alloc's); NIL
bi_bank_free:
            jsr         y_enter
            ldy         #2
            lda         #0
            jsr         hd_byte
            bcc         @lb585
            jmp         hd_rts
@lb585:
            pha
            ldy         #0
            lda         #0
            jsr         hd_byte
            plx
            bcc         @lb584
            jmp         hd_rts
@lb584:
            jsr         BANKS_FREE
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_nil

; (bank-read bank offset n): n bytes of a bank of the task's (offset 0-8191, those n within it), a list
bi_bank_read:
            jsr         y_enter
            stz         hd_seg
            ldy         #0
            lda         #0
            jsr         hd_byte
            bcc         @lb583
            jmp         hd_rts
@lb583:
            sta         hd_bank
            ldy         #2
            jmp         hd_read

; (bank-write bank offset bytes): the bytes (a list of 0-255) into a bank of the task's at offset; NIL
bi_bank_write:
            jsr         y_enter
            stz         hd_seg
            ldy         #0
            lda         #0
            jsr         hd_byte
            bcc         @lb582
            jmp         hd_rts
@lb582:
            sta         hd_bank
            ldy         #2
            jmp         hd_write

; (seg-create banks): a shared segment made of so many banks (1-128); its number
bi_seg_create:
            jsr         y_enter
            ldy         #0
            lda         #129
            jsr         hd_byte
            bcc         @lb581
            jmp         hd_rts
@lb581:
            jsr         SEG_CREATE
            bcc         :+
            jmp         y_syserr0
:
            ldx         #0
            jmp         y_fix

; (seg-attach seg), (seg-detach seg): the segment attached to the task, or not; NIL
bi_seg_attach:
            lda         #0
            bra         hd_segcall
bi_seg_detach:
            lda         #1
hd_segcall:
            sta         hd_a
            jsr         y_enter
            ldy         #0
            lda         #16
            jsr         hd_byte
            bcs         hd_rts2
            ldx         hd_a
            bne         :+
            jsr         SEG_ATTACH
            bra         @done
:
            jsr         SEG_DETACH
@done:
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_nil
hd_rts2:
            clc
            rts

; (seg-read seg bank offset n): n bytes of a segment's bank (attached; its bank 0 on) at offset, a list
bi_seg_read:
            jsr         y_enter
            jsr         hd_segbank
            bcs         hd_rts2
            ldy         #4
            jmp         hd_read

; (seg-write seg bank offset bytes): the bytes (a list of 0-255) into a segment's bank at offset; NIL
bi_seg_write:
            jsr         y_enter
            jsr         hd_segbank
            bcs         hd_rts2
            ldy         #4
            jmp         hd_write

; The window's settings for the segment's bank, arguments 0 and 1 (SEG_MAP's).  OUT: C = 1, ex the error, if not
hd_segbank:
            ldy         #2
            lda         #0
            jsr         hd_byte
            bcs         @rts
            pha
            ldy         #0
            lda         #16
            jsr         hd_byte
            plx
            bcs         @rts
            jsr         SEG_MAP
            bcc         :+
            jsr         y_syserr0
            sec
@rts:
            rts
:
            sta         hd_u
            stx         hd_bank
            lda         #1
            sta         hd_seg
            clc
            rts

; The window to hd_bank (hd_seg: a segment's, U first)
hd_map:
            lda         hd_seg
            beq         :+
            lda         hd_u
            sta         U_REGISTER
:
            lda         hd_bank
            sta         $00
            rts

; The arguments from .Y, an offset and a count (within a bank: 8K), read from the window's bank, a list (ex)
hd_read:
            phy
            lda         #0
            jsr         hd_word                             ; (The offset ...
            ply
            bcs         @rts
            jsr         hd_offset
            bcs         @rts
            iny
            iny
            lda         #0
            jsr         hd_word                             ;   and the count)
            bcs         @rts
            lda         n_w32 + 2
            ora         n_w32 + 3
            bne         @range
            lda         n_w32                               ; (Its end $A000 at most)
            sta         hd_n
            clc
            adc         hq
            sta         hd_w
            lda         n_w32 + 1
            sta         hd_n + 1
            adc         hq + 1
            bcs         @range
            cmp         #$A0
            bcc         :+
            bne         @range
            lda         hd_w
            bne         @range
:
            stz         cf
            stz         cf + 1
@byte:
            lda         hd_n
            ora         hd_n + 1
            beq         @done
            jsr         hd_map
            lda         (hq)
            jsr         hd_fix
            lda         hv
            ldx         hv + 1
            jsr         y_append
            inc         hq
            bne         :+
            inc         hq + 1
:
            lda         hd_n
            bne         :+
            dec         hd_n + 1
:
            dec         hd_n
            bra         @byte
@done:
            MOVW        ex, cf
            clc
@rts:
            rts
@range:
            LDAX        s_hrange
            jsr         y_fnerr
            clc
            rts

; The arguments from .Y, an offset and bytes (a list of 0-255), written to the window's bank; NIL (ex)
hd_write:
            phy
            lda         #0
            jsr         hd_word
            ply
            bcs         @rts
            jsr         hd_offset
            bcs         @rts
            iny
            iny
            jsr         y_argy                              ; (The bytes: a list)
            jsr         q_conses
            bcs         @bad
            sta         y_v
            stx         y_v + 1
@byte:
            lda         y_v
            ora         y_v + 1
            beq         @done
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            lsr                                             ; (A byte: a fixnum, 0-255)
            bcc         @bad
            cpx         #2
            bcs         @bad
            cpx         #1
            bne         :+
            ora         #$80
:
            pha
            lda         hq + 1                              ; (Within the bank)
            cmp         #$A0
            pla
            bcs         @range
            pha
            jsr         hd_map
            pla
            sta         (hq)
            inc         hq
            bne         :+
            inc         hq + 1
:
            lda         y_v
            ldx         y_v + 1
            ldy         #1
            jsr         cell_get
            sta         y_v
            stx         y_v + 1
            bra         @byte
@done:
            jmp         y_nil
@bad:
            LDAX        s_hbytes
            jsr         y_fnerr
            clc
@rts:
            rts
@range:
            LDAX        s_hrange
            jsr         y_fnerr
            clc
            rts

; hq = the window's address for the offset at n_w32 (0-8191).  OUT: C = 1, ex the error, if it's past the bank
hd_offset:
            lda         n_w32 + 2
            ora         n_w32 + 3
            bne         @range
            lda         n_w32 + 1
            cmp         #$20
            bcs         @range
            ora         #$80
            sta         hq + 1
            lda         n_w32
            sta         hq
            clc
            rts
@range:
            LDAX        s_hrange
            jsr         y_fnerr
            sec
            rts

; (free): the RAM, as free shows it, in K: a hash of :ram (a task's own: its banks'), :shared (what segments can
; have), :used (what they have), :free, :segments (those there are)
bi_free:
            jsr         y_enter
            stz         cf
            stz         cf + 1
            jsr         BANKS                               ; (A task's banks, 8K each)
            jsr         hd_kb
            LDAX        s_kram8
            jsr         y_entry
            jsr         SEGINFO
            sta         hd_a
            stx         hd_x
            MOVW        hd_w, r0
            lda         hd_a
            jsr         hd_kb
            LDAX        s_kshared
            jsr         y_entry
            lda         hd_x
            jsr         hd_kb
            LDAX        s_kused
            jsr         y_entry
            lda         hd_a
            sec
            sbc         hd_x
            jsr         hd_kb
            LDAX        s_kfree8
            jsr         y_entry
            ldx         #0                                  ; (The segments: the bits set)
            stz         hd_i
:
            lsr         hd_w + 1
            ror         hd_w
            bcc         :+
            inc         hd_i
:
            inx
            cpx         #16
            bcc         :--
            lda         hd_i
            jsr         hd_fix
            LDAX        s_ksegs
            jsr         y_entry
            FARN        4, h_fromlist
            bcc         :+
            jmp         y_nomem
:
            MOVW        ex, hv
            clc
            rts

; hv = .A banks in K (8 each)
hd_kb:
            stz         n_w32 + 1
            asl
            rol         n_w32 + 1
            asl
            rol         n_w32 + 1
            asl
            rol         n_w32 + 1
            sta         n_w32
            stz         n_w32 + 2
            stz         n_w32 + 3
            jmp         y_u32

; ---- Namespaces

; (bind new old [:before | :after] [:create]): new bound at old (BIND): in its place, or a union with what's there,
; new first (:before) or last (:after); :create, a create in the union comes to new; NIL
bi_bindns:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         hd_rts3
            ldy         #2
            ldx         #1
            jsr         y_patharg
            bcs         hd_rts3
            ldy         #4
            jsr         hd_flags
            bcs         hd_rts3
            lda         hd_spec
            bne         hd_badfl
            LDR         r0, y_path
            LDR         r1, y_path2
            lda         hd_a
            jsr         BIND
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_nil
hd_badfl:
            LDAX        s_hflags
            jsr         y_fnerr
hd_rts3:
            clc
            rts

; (mount dev old [spec] [:before | :after] [:create]): a device's server at old (MOUNT; dev "#f", a spec "x" which
; of its trees: the ROM disk), as bind puts new; NIL
bi_mount:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         hd_rts3
            lda         y_path                              ; (dev: "#" and a letter)
            cmp         #'#'
            bne         @dev
            lda         y_path + 1
            beq         @dev
            sta         hd_x
            lda         y_path + 2
            bne         @dev
            ldy         #2
            ldx         #1
            jsr         y_patharg
            bcs         hd_rts3
            ldy         #4
            jsr         hd_flags
            bcs         hd_rts3
            ldy         hd_spec                             ; (Its spec, a string among the flags)
            beq         :+
            ldx         #0
            jsr         y_patharg
            bcs         hd_rts3
:
            stz         r0
            stz         r0 + 1
            lda         hd_spec
            beq         :+
            LDR         r0, y_path
:
            LDR         r1, y_path2
            ldx         hd_x
            lda         hd_a
            jsr         MOUNT
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_nil
@dev:
            LDAX        s_hdev
            jsr         y_fnerr
            clc
            rts

; (unmount old [new]): what's bound or mounted at old undone (UNMOUNT): all of it, or new's member; NIL
bi_unmount:
            jsr         y_enter
            ldy         #0
            ldx         #1
            jsr         y_patharg
            bcs         hd_rts3
            lda         cn
            cmp         #2
            bcc         :+
            ldy         #2
            ldx         #0
            jsr         y_patharg
            bcc         @lb602
            jmp         hd_rts3
@lb602:
:
            stz         r0
            stz         r0 + 1
            lda         cn
            cmp         #2
            bcc         :+
            LDR         r0, y_path
:
            LDR         r1, y_path2
            jsr         UNMOUNT
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_nil

; The arguments from .Y on: :before, :after, :create (hd_a: MREPL, MBEFORE or MAFTER, | MCREATE); and a string
; among them, mount's spec (hd_spec: its argument's offset; 0, none).  OUT: C = 1, ex the error, if one's neither
hd_flags:
            stz         hd_a
            stz         hd_spec
@arg:
            tya
            lsr
            cmp         cn
            bcs         @done
            sty         hd_i
            jsr         y_argy
            sta         y_v
            stx         y_v + 1
            jsr         kind_of
            cmp         #PK_ATOM
            beq         @atom
            cmp         #PK_STRING
            bne         @bad
            lda         hd_spec
            bne         @bad
            lda         hd_i
            sta         hd_spec
            bra         @next
@atom:
            LDHQ        s_flags, 0
            jsr         hd_which
            bcs         @bad
            lda         hd_flbit,x
            ora         hd_a
            sta         hd_a
@next:
            ldy         hd_i
            iny
            iny
            bra         @arg
@done:
            clc
            rts
@bad:
            LDAX        s_hflags
            jsr         y_fnerr
            sec
            rts

; .X = which of the zero-terminated names at hq (one after another) names the atom y_v.  OUT: C = 1 if none does
hd_which:
            ldx         #0
            MOVW        hd_w, hq
@name:
            phx
            MOVW        hq, hd_w
            txa
            jsr         hd_nth
            plx
            lda         hv
            ora         hv + 1
            beq         @none
            lda         hv
            cmp         y_v
            bne         @next
            lda         hv + 1
            cmp         y_v + 1
            bne         @next
            clc
            rts
@next:
            inx
            bra         @name
@none:
            sec
            rts

; (ns): the namespace's binds and mounts, a list of hashes: :old (where), :new (what: a device's path, "#fx/lib"),
; :create (T: a create in the union comes to it); in the kernel's order, a union's members first to last
bi_ns:
            jsr         y_enter
            stz         cf
            stz         cf + 1
            stz         hd_i
@entry:
            LDR         r0, y_buf
            lda         #$FF
            ldx         hd_i
            jsr         NSINFO
            bcc         @lb603
            jmp         @done
@lb603:
            jsr         hd_begin
            LDAX        y_buf + NI_FROM
            ldy         #255
            jsr         hd_zstr
            LDAX        s_kold
            jsr         y_entry
            lda         #'#'                                ; (New: "#", the device, its spec, "/", the path)
            sta         y_buf + 144
            lda         y_buf + NI_DEV
            sta         y_buf + 145
            ldx         #146
            ldy         #0
:
            lda         y_buf + NI_SPEC,y
            beq         :+
            sta         y_buf,x
            inx
            iny
            cpy         #8
            bcc         :-
:
            lda         #'/'
            sta         y_buf,x
            inx
            ldy         #0
:
            lda         y_buf + NI_PATH,y
            sta         y_buf,x
            beq         :+
            inx
            iny
            cpy         #64
            bcc         :-
            stz         y_buf,x
:
            LDAX        y_buf + 144
            ldy         #255
            jsr         hd_zstr
            LDAX        s_knew
            jsr         y_entry
            stz         hv
            stz         hv + 1
            lda         y_buf + NI_FLAGS
            and         #MCREATE
            beq         :+
            lda         #<T_VAL
            sta         hv
:
            LDAX        s_kcreate
            jsr         y_entry
            jsr         hd_end
            lda         hv
            ldx         hv + 1
            jsr         y_append
            inc         hd_i
            beq         @lb601
            jmp         @entry
@lb601:
@done:
            MOVW        ex, cf
            clc
            rts

; ---- Notes

; (note task n), (note-group group n): the note n (:interrupt, :kill, :hangup, :alarm, :brk; or its number, 16-31 a
; program's own) sent to the task, or to each task in the note group; NIL
bi_note:
            lda         #0
            bra         hd_note
bi_note_group:
            lda         #NOTE_GROUP
hd_note:
            sta         hd_a
            jsr         y_enter
            ldy         #0
            lda         #16
            jsr         hd_byte
            bcs         hd_rts4
            ora         hd_a
            sta         hd_t
            ldy         #2
            jsr         hd_noteno
            bcs         hd_rts4
            tax
            lda         hd_t
            jsr         NOTE
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_nil
hd_rts4:
            clc
            rts

; The argument at .Y a note: an atom (its number: 1 :interrupt ... 5 :brk) or a number (1-31): .A.  OUT: C = 1, ex
; the error, if it isn't one
hd_noteno:
            sty         hd_i
            jsr         y_argy
            sta         y_v
            stx         y_v + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         @number
            LDHQ        s_notes, 0
            jsr         hd_which
            bcs         @bad
            inx
            txa
            clc
            rts
@number:
            ldy         hd_i
            lda         #32
            jsr         hd_byte
            bcs         @rts
            cmp         #1
            bcc         @bad
            clc
@rts:
            rts
@bad:
            LDAX        s_hnote
            jsr         y_fnerr
            sec
            rts

; (on-note f): f called with each note that comes (but a kill) at the next call, the note an atom (:interrupt,
; :hangup, :alarm, :brk) or its number (16-31, a program's own): its value T to go on, NIL for the note's default
; (Ctrl-C's: the error :intr; another's, hylang's end, as the system's default ends it); (on-note NIL): the
; defaults.  NIL
bi_on_note:
            jsr         y_arg0
            php
            sei
            sta         nt_f
            stx         nt_f + 1
            plp
            jmp         y_nil

; ---- Keys

; (key): the next key, raw (as it comes, unechoed): a character, or an atom for the terminal's keys (:up, :down,
; :right, :left, :home, :end, :ins, :del, :pgup, :pgdn, :f1 ... :f12).  Raw till the next line is read (the
; prompt's, read-line's).  NIL at stdin's end (Ctrl-C: the error :intr, at the next call)
bi_key:
            jsr         y_enter
            jsr         kd_rawon
            FARN        1, getc_in
            bcc         kd_val
            jmp         y_nil

; ex = the key .A: a character, or the atom of a terminal's key ($80-$95)
kd_val:
            cmp         #KEY_UP
            bcc         @char
            cmp         #KEY_F12 + 1
            bcs         @char
            sbc         #KEY_UP - 1                         ; (C = 0: less 1 more)
            LDHQ        s_keys, 0
            jsr         hd_nth
            MOVW        ex, hv
            clc
            rts
@char:
            stz         ex + 1
            asl
            sta         ex
            rol         ex + 1
            lda         ex + 1
            ora         #>CHAR0
            sta         ex + 1
            clc
            rts

; (key?): T if a key is waiting (raw, as key's)
bi_keyp:
            jsr         y_enter
            ldx         ipos
            cpx         ilen
            bcc         @yes
            jsr         kd_rawon
            lda         kd_nb
            beq         @no
            LDR         r0, ibuf
            LDR         r1, 1
            lda         kd_nb
            dec         a
            jsr         READ
            bcs         @no
            cmp         #0
            beq         @no
            sta         ilen
            stz         ipos
@yes:
            jmp         y_t
@no:
            jmp         y_nil

; The console raw (consctl's rawon, kept open: kd_ctl), and a read of it that doesn't wait (kd_nb): fds + 1, 0 if
; they're not (stdin isn't the console's)
kd_rawon:
            lda         kd_ctl
            bne         @rts
            ldx         #s_consctl_n
:
            lda         s_consctl,x
            sta         y_path,x
            dex
            bpl         :-
            LDR         r0, y_path
            lda         #O_WRITE
            jsr         OPEN
            bcs         @rts
            inc         a
            sta         kd_ctl
            ldx         #5
:
            lda         s_rawon,x
            sta         y_buf,x
            dex
            bpl         :-
            LDR         r0, y_buf
            LDR         r1, 5
            lda         kd_ctl
            dec         a
            jsr         WRITE
            ldx         #s_consctl_n - 3                    ; (/dev/cons, read without waiting)
            stz         y_path,x
            LDR         r0, y_path
            lda         #O_READ | O_NONBLOCK
            jsr         OPEN
            bcs         @rts
            inc         a
            sta         kd_nb
@rts:
            rts

.segment "RODATA5"
s_hflags:   .byte       "' expects :before, :after or :create", 0
s_hdev:     .byte       "' expects a device: # and its letter", 0
s_hnote:    .byte       "' expects a note: :interrupt, :kill, :hangup, :alarm, :brk, or 1-31", 0
s_kold:     .byte       "old", 0
s_knew:     .byte       "new", 0
s_kcreate:  .byte       "create", 0
s_flags:    .byte       "before", 0, "after", 0, "create", 0, 0
hd_flbit:   .byte       MBEFORE, MAFTER, MCREATE
s_notes:    .byte       "interrupt", 0, "kill", 0, "hangup", 0, "alarm", 0, "brk", 0, 0
s_keys:     .byte       "up", 0, "down", 0, "right", 0, "left", 0, "home", 0, "end", 0, "ins", 0, "del", 0
            .byte       "pgup", 0, "pgdn", 0, "f1", 0, "f2", 0, "f3", 0, "f4", 0, "f5", 0, "f6", 0, "f7", 0
            .byte       "f8", 0, "f9", 0, "f10", 0, "f11", 0, "f12", 0, 0
s_consctl:  .byte       "/dev/consctl", 0
s_consctl_n = * - s_consctl - 1
s_rawon:    .byte       "rawon", 0
s_hwhole:   .byte       "' expects a whole number", 0
s_hrange:   .byte       "' was given a number out of its range", 0
s_hcode:    .byte       "' expects an error's code", 0
s_hbytes:   .byte       "' expects a list of bytes (0-255)", 0
s_kabi:     .byte       "abi", 0
s_kram:     .byte       "ram-modules", 0
s_kfree:    .byte       "free-tasks", 0
s_ktype:    .byte       "type", 0
s_kbank:    .byte       "bank", 0
s_kbanks:   .byte       "banks", 0
s_ktask:    .byte       "task", 0
s_kstate:   .byte       "state", 0
s_kparent:  .byte       "parent", 0
s_kcpu:     .byte       "cpu", 0
s_kgroup:   .byte       "group", 0
s_kargs:    .byte       "args", 0
s_kram8:    .byte       "ram", 0
s_kshared:  .byte       "shared", 0
s_kused:    .byte       "used", 0
s_kfree8:   .byte       "free", 0
s_ksegs:    .byte       "segments", 0
s_types:    .byte       "program", 0, "driver", 0, "library", 0, "module", 0, 0
s_states:   .byte       "ready", 0, "wait", 0, "call", 0, "idle", 0, "new", 0, "sleep", 0, "blocked", 0, "event", 0, 0
