; ****************************************************************************
; buffers.inc - hylang's buffers (danlang's ffafc79: reference.md, section 4's Buffers), in its sixth bank: bytes,
; changed in place (so shared, as a hash is: one value wherever it's given or kept).  A buffer (PK_BUFFER) is a
; string's cell, its length and its blob, 8,184 bytes at most (a blob's).  buffer makes one (a count's, filled; a
; string's bytes; a list's; another's, a copy), buffer-get and a buffer called, (b i), read a byte (buf_at, the
; evaluator's), buffer-put writes one, buffer-fill and buffer-copy many (two buffers' blobs reached in turn, through
; bf_chunk); buffer? is nums.inc's, read-buffer and write-bytes streams.inc's, and len, bytes, from-bytes, eq, cmp and
; the printer take a buffer as a string.  A byte is an integer 0-255, or a character (its code); anything else is an
; error, its code :inval.  An index is from 0; a count past the end takes what there is.

BUF_MAX         = $1FF8         ; (A blob's most: 8,184 bytes)
BF_CHUNK        = 64            ; (bf_xfer's bytes at a time)

.bss
bf_fn:      .res        2                                   ; The built-in's name (in this bank): its errors'
bf_b:       .res        2                                   ; (A buffer: its value, length, blob ...
bf_len:     .res        2
bf_blob:    .res        2
bf_i:       .res        2                                   ;   an index, a count ...
bf_n:       .res        2
bf_byte:    .res        1                                   ;   a byte)
bf_sb:      .res        2                                   ; (bf_xfer's: from a blob at an offset, to one at one,
bf_so:      .res        2                                   ;   backward if bf_back)
bf_db:      .res        2
bf_do:      .res        2
bf_back:    .res        1
bf_k:       .res        1
bf_endok:   .res        1                                   ; (bf_index's: <> 0, the end too)
bf_to:      .res        2                                   ; (buffer-copy's: to, and its room from at)
bf_room:    .res        2
bf_l:       .res        2                                   ; (A list walked)
bf_chunk:   .res        BF_CHUNK

.segment "CODE6"

; ---- The abort point and errors

; A bank-6 entry's abort point (y_enter's, its first thing: jsr bf_enter); .A, .X its name (in this bank)
bf_enter:
            sta         bf_fn
            stx         bf_fn + 1
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

; An error's message begun, its code :inval (bf_begin_inval) or none: "'buffer-put'"
bf_begin_inval:
            lda         #E_INVAL
            FARN        5, y_errcode
            bra         :+
bf_begin:
            stz         y_code
            stz         y_code + 1
:
            FARN        5, y_begin
            lda         #$27
            jsr         out_byte
            lda         bf_fn
            ldx         bf_fn + 1
            jsr         out_text
            lda         #$27
            jmp         out_byte

; The value .A, .X out, as print shows it
bf_value:
            ldy         #1
            FARN        2, print_val
            rts

; The message ended: ex the error; back from the entry
bf_end:
            FARN        5, y_end
            ldx         y_sp
            txs
            lda         y_rsp
            sta         rsp
            clc
            rts

; "'fn': v isn't a byte (0-255)", :inval
bf_e_byte:
            pha
            phx
            jsr         bf_begin_inval
            LDAX        s_bfcolon
            jsr         out_text
            plx
            pla
            jsr         bf_value
            LDAX        s_bfbyte
            jsr         out_text
            jmp         bf_end

; "'fn' expects a buffer, not v"
bf_e_buffer:
            pha
            phx
            jsr         bf_begin
            LDAX        s_bfexpects
            jsr         out_text
            plx
            pla
            jsr         bf_value
            jmp         bf_end

; "'fn': WHAT must be a whole number, not v" (WHAT at bf_n's place: .Y's text, s_bfindex, s_bfcount, s_bfsize)
bf_e_whole:
            pha
            phx
            phy
            jsr         bf_begin
            LDAX        s_bfcolon
            jsr         out_text
            ply
            ldx         #>s_bfindex
            tya
            jsr         out_text
            LDAX        s_bfwhole
            jsr         out_text
            plx
            pla
            jsr         bf_value
            jmp         bf_end

; "'fn': index v is outside the buffer"
bf_e_index:
            pha
            phx
            jsr         bf_begin
            LDAX        s_bfindexv
            jsr         out_text
            plx
            pla
            jsr         bf_value
            LDAX        s_bfoutside
            jsr         out_text
            jmp         bf_end

; "'fn': a count can't be negative"
bf_e_count:
            jsr         bf_begin
            LDAX        s_bfnegative
            jsr         out_text
            jmp         bf_end

; The message at .A, .X: "'fn' message" (no code)
bf_e_text:
            pha
            phx
            jsr         bf_begin
            plx
            pla
            jsr         out_text
            jmp         bf_end

; ---- Arguments

; .A, .X = the argument at .Y
bf_arg:
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            rts

; The argument at .Y a buffer: bf_b, bf_len, bf_blob; else the error
bf_buffer:
            jsr         bf_arg
bf_buffer_ax:
            sta         bf_b
            stx         bf_b + 1
            jsr         kind_of
            cmp         #PK_BUFFER
            beq         bf_cell
            lda         bf_b
            ldx         bf_b + 1
            jmp         bf_e_buffer
; bf_b's length and blob (bf_len, bf_blob)
bf_cell:
            lda         bf_b
            ldx         bf_b + 1
            jsr         deref
            lda         (hp)
            sta         bf_len
            ldy         #1
            lda         (hp),y
            sta         bf_len + 1
            iny
            lda         (hp),y
            sta         bf_blob
            iny
            lda         (hp),y
            sta         bf_blob + 1
            rts

; .A, .X a byte (an integer 0-255, or a character): bf_byte; else the error
bf_tobyte:
            pha
            lsr
            pla
            bcc         @char
            cpx         #2                                  ; (A fixnum 0-255: its high byte 0 or 1)
            bcs         @no
            pha
            txa
            lsr
            pla
            ror
            sta         bf_byte
            rts
@char:
            cpx         #>CHAR0
            bcc         @no
            cpx         #>CHAR0 + 2
            bcs         @no
            pha
            txa
            lsr
            pla
            ror
            sta         bf_byte
            rts
@no:
            jmp         bf_e_byte

; The argument at .Y a whole number (-2^31 to 2^32 - 1): n_w32; else the error, WHAT the text at .A's place
; (s_bfindex's page)
bf_whole:
            pha
            jsr         bf_arg
            sta         bf_l
            stx         bf_l + 1
            FARN        3, n_to_i32
            pla
            bcs         :+
            rts
:
            tay
            lda         bf_l
            ldx         bf_l + 1
            jmp         bf_e_whole

; The argument at .Y an index of bf_len bytes (0 to its length - 1; bf_index_end: to its length too): bf_i
bf_index_end:
            lda         #1
            bra         bf_index
bf_index_in:
            lda         #0
bf_index:
            sta         bf_endok
            lda         #<s_bfindex
            jsr         bf_whole
            lda         n_w32 + 3                           ; (Negative, or 64K and more: outside)
            ora         n_w32 + 2
            bne         @out
            lda         n_w32
            sta         bf_i
            lda         n_w32 + 1
            sta         bf_i + 1
            lda         bf_i                                ; (Below its length: in)
            cmp         bf_len
            lda         bf_i + 1
            sbc         bf_len + 1
            bcc         @in
            lda         bf_i                                ; (Its length: the end, in if it may be)
            cmp         bf_len
            bne         @out
            lda         bf_i + 1
            cmp         bf_len + 1
            bne         @out
            lda         bf_endok
            bne         @in
@out:
            lda         bf_l
            ldx         bf_l + 1
            jmp         bf_e_index
@in:
            rts

; The argument at .Y a count (0 on) of what's left from bf_i: bf_n, at most bf_len - bf_i
bf_count:
            lda         #<s_bfcount
            jsr         bf_whole
            lda         n_w32 + 3
            bpl         :+
            jmp         bf_e_count
:
            sec                                             ; (What's left: bf_len - bf_i)
            lda         bf_len
            sbc         bf_i
            sta         bf_n
            lda         bf_len + 1
            sbc         bf_i + 1
            sta         bf_n + 1
            lda         n_w32 + 2                           ; (64K and more: what's left)
            ora         n_w32 + 3
            bne         @rts
            lda         n_w32
            cmp         bf_n
            lda         n_w32 + 1
            sbc         bf_n + 1
            bcs         @rts
            lda         n_w32
            sta         bf_n
            lda         n_w32 + 1
            sta         bf_n + 1
@rts:
            rts

; ---- Bytes

; hp = the address of byte bf_i of the blob .A, .X (its bank in the window)
bf_at:
            jsr         blob_data
            clc
            lda         hp
            adc         bf_i
            sta         hp
            lda         hp + 1
            adc         bf_i + 1
            sta         hp + 1
            rts

; bf_n bytes from the blob bf_sb at bf_so to the blob bf_db at bf_do, through bf_chunk, BF_CHUNK at a time; from the
; end first if bf_back (an overlap: the same buffer, to a place after the one from)
bf_xfer:
            lda         bf_n
            ora         bf_n + 1
            bne         :+
            rts
:
            lda         bf_n + 1                            ; (bf_k: this chunk's bytes)
            bne         @full
            lda         bf_n
            cmp         #BF_CHUNK
            bcc         :+
@full:
            lda         #BF_CHUNK
:
            sta         bf_k
            sec                                             ; (bf_n less it: what's left after, and from the end,
            lda         bf_n                                ;   this chunk's place past the offsets)
            sbc         bf_k
            sta         bf_n
            bcs         :+
            dec         bf_n + 1
:
            lda         bf_back
            bne         @back
            MOVW        bf_i, bf_so                         ; (Forward: this chunk at the offsets, then past it)
            jsr         @in
            MOVW        bf_i, bf_do
            jsr         @out
            clc
            lda         bf_so
            adc         bf_k
            sta         bf_so
            bcc         :+
            inc         bf_so + 1
:
            clc
            lda         bf_do
            adc         bf_k
            sta         bf_do
            bcc         bf_xfer
            inc         bf_do + 1
            bra         bf_xfer
@back:
            clc                                             ; (Backward: this chunk at the offsets plus what's left)
            lda         bf_so
            adc         bf_n
            sta         bf_i
            lda         bf_so + 1
            adc         bf_n + 1
            sta         bf_i + 1
            jsr         @in
            clc
            lda         bf_do
            adc         bf_n
            sta         bf_i
            lda         bf_do + 1
            adc         bf_n + 1
            sta         bf_i + 1
            jsr         @out
            jmp         bf_xfer
@in:
            lda         bf_sb
            ldx         bf_sb + 1
            jsr         bf_at
            ldy         #0
:
            lda         (hp),y
            sta         bf_chunk,y
            iny
            cpy         bf_k
            bcc         :-
            rts
@out:
            lda         bf_db
            ldx         bf_db + 1
            jsr         bf_at
            ldy         #0
:
            lda         bf_chunk,y
            sta         (hp),y
            iny
            cpy         bf_k
            bcc         :-
            rts

; A new buffer of hn bytes (BUF_MAX at most, else the error): hv, bf_b; its length and blob, bf_len and bf_blob
bf_new:
            lda         hn + 1
            cmp         #>BUF_MAX
            bcc         @ok
            bne         @big
            lda         hn
            cmp         #<BUF_MAX + 1
            bcc         @ok
@big:
            lda         hn                                  ; (Its count, a number)
            sta         n_w32
            lda         hn + 1
            sta         n_w32 + 1
            stz         n_w32 + 2
            stz         n_w32 + 3
            FARN        5, y_u32
            lda         hv
            ldx         hv + 1
            jmp         bf_e_size
@ok:
            lda         #PK_BUFFER
            jsr         kind_alloc
            bcc         :+
            FARN        5, y_nomem
:
            MOVW        bf_b, hv
            jmp         bf_cell

; "'buffer': v bytes is outside 0 to 8184"
bf_e_size:
            pha
            phx
            jsr         bf_begin
            LDAX        s_bfcolon
            jsr         out_text
            plx
            pla
            jsr         bf_value
            LDAX        s_bfbytes
            jsr         out_text
            jmp         bf_end

; ---- The built-ins

; (buffer n [fill]), (buffer s), (buffer l), (buffer b): a buffer of n bytes, each fill (0); of a string's bytes (a
; character's byte), a list's (each a byte), another buffer's (a copy)
bi_buffer:
            LDAX        s_nbuffer
            jsr         bf_enter
            ldy         #0
            jsr         bf_arg
            sta         bf_l
            stx         bf_l + 1
            jsr         kind_of
            cmp         #PK_STRING
            bne         @lb745
            jmp         @string
@lb745:
            cmp         #PK_BUFFER
            bne         @lb744
            jmp         @string
@lb744:
            lda         bf_l
            ldx         bf_l + 1
            jsr         q_conses
            bcs         @notlist
            sta         bf_l
            stx         bf_l + 1
            jmp         @list
@notlist:
            lda         bf_l                                ; (A character: its byte; a number, a count)
            lsr
            bcs         @count
            lda         bf_l + 1
            cmp         #>CHAR0
            bcc         @count
            cmp         #>CHAR0 + 2
            bcs         @count
            jsr         @one
            lda         bf_l
            ldx         bf_l + 1
            jsr         bf_tobyte
            lda         #1
            sta         hn
            stz         hn + 1
            jsr         bf_new
            stz         bf_i
            stz         bf_i + 1
            lda         bf_blob
            ldx         bf_blob + 1
            jsr         bf_at
            lda         bf_byte
            sta         (hp)
            jmp         @done
@count:
            ldy         #0                                  ; (A count, 0 to BUF_MAX: hn)
            lda         #<s_bfsize
            jsr         bf_whole
            lda         n_w32 + 2
            ora         n_w32 + 3
            bne         @size
            lda         n_w32
            sta         hn
            lda         n_w32 + 1
            sta         hn + 1
            stz         bf_byte
            lda         cn                                  ; (Its fill)
            cmp         #2
            bcc         :+
            ldy         #2
            jsr         bf_arg
            jsr         bf_tobyte
:
            jsr         bf_new
            lda         bf_blob
            ldx         bf_blob + 1
            jsr         blob_data
            MOVW        bf_n, bf_len
@fill:
            lda         bf_n
            ora         bf_n + 1
            bne         @lb743
            jmp         @done
@lb743:
            lda         bf_byte
            sta         (hp)
            inc         hp
            bne         :+
            inc         hp + 1
:
            lda         bf_n
            bne         :+
            dec         bf_n + 1
:
            dec         bf_n
            bra         @fill
@size:
            ldy         #0                                  ; (Below 0, or 64K and more: as it is)
            jsr         bf_arg
            jmp         bf_e_size
@string:
            jsr         @one                                ; (A string's bytes, a buffer's: its blob's, copied)
            lda         bf_l
            ldx         bf_l + 1
            jsr         deref
            lda         (hp)
            sta         hn
            ldy         #1
            lda         (hp),y
            sta         hn + 1
            jsr         bf_new
            lda         bf_l                                ; (Its blob again: the new one may have moved it)
            ldx         bf_l + 1
            jsr         deref
            ldy         #2
            lda         (hp),y
            sta         bf_sb
            iny
            lda         (hp),y
            sta         bf_sb + 1
            MOVW        bf_db, bf_blob
            stz         bf_so
            stz         bf_so + 1
            stz         bf_do
            stz         bf_do + 1
            stz         bf_back
            MOVW        bf_n, bf_len
            jsr         bf_xfer
            jmp         @done
@list:
            jsr         @one                                ; (A list: each a byte, checked first; then made)
            stz         hn
            stz         hn + 1
            MOVW        bf_i, bf_l
:
            lda         bf_i
            ora         bf_i + 1
            beq         :+
            lda         bf_i
            ldx         bf_i + 1
            ldy         #0
            jsr         cell_get
            jsr         bf_tobyte
            inc         hn
            bne         @lnext
            inc         hn + 1
@lnext:
            lda         bf_i
            ldx         bf_i + 1
            ldy         #1
            jsr         cell_get
            sta         bf_i
            stx         bf_i + 1
            bra         :-
:
            jsr         bf_new
            stz         bf_i
            stz         bf_i + 1
@lput:
            lda         bf_l
            ora         bf_l + 1
            beq         @done
            lda         bf_l
            ldx         bf_l + 1
            ldy         #0
            jsr         cell_get
            jsr         bf_tobyte
            lda         bf_blob
            ldx         bf_blob + 1
            jsr         bf_at
            lda         bf_byte
            sta         (hp)
            inc         bf_i
            bne         :+
            inc         bf_i + 1
:
            lda         bf_l
            ldx         bf_l + 1
            ldy         #1
            jsr         cell_get
            sta         bf_l
            stx         bf_l + 1
            bra         @lput
@done:
            MOVW        ex, bf_b
            clc
            rts
@one:
            lda         cn                                  ; (A fill only with a count)
            cmp         #2
            bcs         :+
            rts
:
            LDAX        s_bffillonly
            jmp         bf_e_text

; (buffer-get b i), as (b i) is: byte i
bi_buffer_get:
            LDAX        s_nbufget
            jsr         bf_enter
            ldy         #0
            jsr         bf_buffer
            ldy         #2
bf_get:
            jsr         bf_index_in
            lda         bf_blob
            ldx         bf_blob + 1
            jsr         bf_at
            lda         (hp)
            ldx         #0
            jsr         fix_ax
            sta         ex
            stx         ex + 1
            clc
            rts

; A buffer called, (b i) (eval.inc's hcall_key): cf the buffer, ex its index's value.  OUT: ex its byte i (or the
; error, buffer-get's); C = 1 if there's no room
buf_at:
            LDAX        s_nbufget
            jsr         bf_enter
            lda         cf
            ldx         cf + 1
            sta         bf_b
            stx         bf_b + 1
            jsr         bf_cell
            lda         ex                                  ; (Its index: as an argument, at argp's place)
            sta         bf_chunk
            lda         ex + 1
            sta         bf_chunk + 1
            lda         argp
            pha
            lda         argp + 1
            pha
            LDAX        bf_chunk
            sta         argp
            stx         argp + 1
            ldy         #0
            jsr         bf_index_in
            pla
            sta         argp + 1
            pla
            sta         argp
            lda         bf_blob
            ldx         bf_blob + 1
            jsr         bf_at
            lda         (hp)
            ldx         #0
            jsr         fix_ax
            sta         ex
            stx         ex + 1
            clc
            rts

; (buffer-put b i byte): its old byte, byte i set
bi_buffer_put:
            LDAX        s_nbufput
            jsr         bf_enter
            ldy         #0
            jsr         bf_buffer
            ldy         #2
            jsr         bf_index_in
            ldy         #4
            jsr         bf_arg
            jsr         bf_tobyte
            lda         bf_blob
            ldx         bf_blob + 1
            jsr         bf_at
            lda         (hp)
            pha
            lda         bf_byte
            sta         (hp)
            pla
            ldx         #0
            jsr         fix_ax
            sta         ex
            stx         ex + 1
            clc
            rts

; (buffer-fill b byte [i [n]]): NIL, the bytes from i (0) on, n of them (to its end), set to byte
bi_buffer_fill:
            LDAX        s_nbuffill
            jsr         bf_enter
            ldy         #0
            jsr         bf_buffer
            ldy         #2
            jsr         bf_arg
            jsr         bf_tobyte
            stz         bf_i
            stz         bf_i + 1
            lda         cn
            cmp         #3
            bcc         :+
            ldy         #4
            jsr         bf_index_end
:
            sec                                             ; (To its end ...
            lda         bf_len
            sbc         bf_i
            sta         bf_n
            lda         bf_len + 1
            sbc         bf_i + 1
            sta         bf_n + 1
            lda         cn                                  ;   or n of them)
            cmp         #4
            bcc         :+
            ldy         #6
            jsr         bf_count
:
            lda         bf_blob
            ldx         bf_blob + 1
            jsr         bf_at
@byte:
            lda         bf_n
            ora         bf_n + 1
            beq         @done
            lda         bf_byte
            sta         (hp)
            inc         hp
            bne         :+
            inc         hp + 1
:
            lda         bf_n
            bne         :+
            dec         bf_n + 1
:
            dec         bf_n
            bra         @byte
@done:
            stz         ex
            stz         ex + 1
            clc
            rts

; (buffer-copy to at from [i [n]]): NIL, from's bytes from i (0) on, n of them (to its end), put in to from at (as
; many as fit); the two may be one buffer, the parts overlapping
bi_buffer_copy:
            LDAX        s_nbufcopy
            jsr         bf_enter
            ldy         #0                                  ; (to, at: bf_to, bf_db, bf_do; its room from at)
            jsr         bf_buffer
            MOVW        bf_to, bf_b
            ldy         #2
            jsr         bf_index_end
            MOVW        bf_db, bf_blob
            MOVW        bf_do, bf_i
            sec
            lda         bf_len
            sbc         bf_i
            sta         bf_room
            lda         bf_len + 1
            sbc         bf_i + 1
            sta         bf_room + 1
            ldy         #4                                  ; (from: bf_sb, bf_so; the bytes, bf_n)
            jsr         bf_buffer
            MOVW        bf_sb, bf_blob
            stz         bf_i
            stz         bf_i + 1
            lda         cn
            cmp         #4
            bcc         :+
            ldy         #6
            jsr         bf_index_end
:
            MOVW        bf_so, bf_i
            sec
            lda         bf_len
            sbc         bf_i
            sta         bf_n
            lda         bf_len + 1
            sbc         bf_i + 1
            sta         bf_n + 1
            lda         cn
            cmp         #5
            bcc         :+
            ldy         #8
            jsr         bf_count
:
            lda         bf_n                                ; (As many as fit)
            cmp         bf_room
            lda         bf_n + 1
            sbc         bf_room + 1
            bcc         :+
            MOVW        bf_n, bf_room
:
            stz         bf_back                             ; (One buffer, to a place after from's: backward)
            lda         bf_to
            cmp         bf_b
            bne         :+
            lda         bf_to + 1
            cmp         bf_b + 1
            bne         :+
            lda         bf_so
            cmp         bf_do
            lda         bf_so + 1
            sbc         bf_do + 1
            bcs         :+
            inc         bf_back
:
            jsr         bf_xfer
            stz         ex
            stz         ex + 1
            clc
            rts

; (read-buffer s b [at [n]]): up to n bytes (to b's end) read from the stream into b from at (0): how many; NIL at
; the stream's end
bi_read_buffer:
            LDAX        s_nrdbuf
            jsr         bf_enter
            ldy         #0
            FARN        5, y_streamarg
            bcc         @lb742
            jmp         @done
@lb742:
            FARN        5, y_canread
            bcc         @lb741
            jmp         @done
@lb741:
            ldy         #2
            jsr         bf_buffer
            stz         bf_i
            stz         bf_i + 1
            lda         cn
            cmp         #3
            bcc         :+
            ldy         #4
            jsr         bf_index_end
:
            sec                                             ; (To its end, or n)
            lda         bf_len
            sbc         bf_i
            sta         bf_n
            lda         bf_len + 1
            sbc         bf_i + 1
            sta         bf_n + 1
            lda         cn
            cmp         #4
            bcc         :+
            ldy         #6
            jsr         bf_count
:
            stz         bf_k                                ; (bf_room: how many read)
            stz         bf_room
            stz         bf_room + 1
@byte:
            lda         bf_n
            ora         bf_n + 1
            beq         @count
            FARN        5, y_getb
            bcs         @end
            pha
            lda         bf_blob
            ldx         bf_blob + 1
            jsr         bf_at
            pla
            sta         (hp)
            inc         bf_i
            bne         :+
            inc         bf_i + 1
:
            inc         bf_room
            bne         :+
            inc         bf_room + 1
:
            lda         bf_n
            bne         :+
            dec         bf_n + 1
:
            dec         bf_n
            bra         @byte
@end:
            cmp         #0                                  ; (An error: the system's)
            beq         :+
            FARN        5, y_endor
            rts
:
            lda         bf_room                             ; (Its end: NIL if none were read)
            ora         bf_room + 1
            bne         @count
            stz         ex
            stz         ex + 1
@done:
            clc
            rts
@count:
            lda         bf_room
            sta         n_w32
            lda         bf_room + 1
            sta         n_w32 + 1
            stz         n_w32 + 2
            stz         n_w32 + 3
            FARN        5, y_u32
            clc
            rts

; write-bytes's buffer (streams.inc's: the argument 1), from i (the argument 2: 0) on, n of them (the argument 3: to
; its end): its bytes out, a message begun (y_begin)
bf_wbytes:
            LDAX        s_nwbytes
            sta         bf_fn
            stx         bf_fn + 1
            ldy         #2
            jsr         bf_buffer
            stz         bf_i
            stz         bf_i + 1
            lda         cn
            cmp         #3
            bcc         :+
            ldy         #4
            jsr         bf_index_end
:
            sec
            lda         bf_len
            sbc         bf_i
            sta         bf_n
            lda         bf_len + 1
            sbc         bf_i + 1
            sta         bf_n + 1
            lda         cn
            cmp         #4
            bcc         :+
            ldy         #6
            jsr         bf_count
:
            FARN        5, y_begin
            lda         bf_blob
            ldx         bf_blob + 1
            jsr         bf_at
@byte:
            lda         bf_n
            ora         bf_n + 1
            beq         @rts
            lda         (hp)
            jsr         out_byte
            inc         hp
            bne         :+
            inc         hp + 1
:
            lda         bf_n
            bne         :+
            dec         bf_n + 1
:
            dec         bf_n
            bra         @byte
@rts:
            rts

.segment "RODATA6"
s_nbuffer:  .byte       "buffer", 0
s_nrdbuf:   .byte       "read-buffer", 0
s_nwbytes:  .byte       "write-bytes", 0
s_nbufget:  .byte       "buffer-get", 0
s_nbufput:  .byte       "buffer-put", 0
s_nbuffill: .byte       "buffer-fill", 0
s_nbufcopy: .byte       "buffer-copy", 0
s_bfcolon:  .byte       ": ", 0
s_bfbyte:   .byte       " isn't a byte (0-255)", 0
s_bfexpects: .byte      " expects a buffer, not ", 0
.align      $100                                            ; (s_bfindex, s_bfcount, s_bfsize: on one page)
s_bfindex:  .byte       "an index", 0
s_bfcount:  .byte       "a count", 0
s_bfsize:   .byte       "a size", 0
s_bfwhole:  .byte       " must be a whole number, not ", 0
s_bfindexv: .byte       ": index ", 0
s_bfoutside: .byte      " is outside the buffer", 0
s_bfnegative: .byte     ": a count can't be negative", 0
s_bfbytes:  .byte       " bytes is outside 0 to 8184", 0
s_bffillonly: .byte     " takes a fill only with a count", 0
