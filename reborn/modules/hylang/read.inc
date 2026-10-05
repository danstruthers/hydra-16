; ****************************************************************************
; read.inc - hylang's reader (phases 2, 3), in its second bank: text to values, as danlang's (its reference.md,
; section 1, and Parser.cs).  read_text reads text (a line, and those that go on with it, each with its LF, then a 0)
; to the S-expression of its items; read_item reads a file's next item (load's), refilling text from it as it goes
; (each time what's left is less than 2 bytes, so a byte and the one after it are always there but at the file's
; end), what's left of a word being read kept.
;   One pass, without tokens: a level for each bracket open (its closer, its list so far and its last cons, so a list
; is built in order), each item made as it's read and put at the end of its level's list.  The first error ends it
; (as the first token in error is danlang's), made an error value; the text's end with a bracket open, or in a here
; string, asks for more (RD_MORE, the closers wanted as danlang's REPL shows them), and the REPL reads it all again
; with its next line.  Numbers, as yet, are the decimal integers a fixnum holds (the rest: phase 5).
;   What it makes may collect: an item is in VR0 till it's in its list, and the levels' lists are marked (rd_roots,
; gc_hook's).  An error, however deep, goes back to read_text's caller through the stack read_text began with.

TEXT_SIZE       = 4096          ; The text read at most: an expression's lines
SCRATCH_SIZE    = TEXT_SIZE     ; A string's bytes (no more than its text's), a name's, a message's
LEVELS          = 255           ; The brackets open at once, at most

RD_DONE         = 0             ; read_text's: the text read (read_item's: an item) ...
RD_MORE         = 1             ;   more wanted ...
RD_ERROR        = 2             ;   an error ...
RD_END          = 3             ;   (read_item's) no more items

.zeropage
rp:         .res        2                                   ; The reader's: the byte it's at ...
re:         .res        2                                   ;   the text's end (its 0) ...
rw:         .res        2                                   ;   a word's first byte, and its length ...
rn:         .res        2
rs:         .res        2                                   ;   where a string's next byte goes (in scratch) ...
rq:         .res        1                                   ;   a here string's quotes ...
rd:         .res        1                                   ;   the level it's at (0: the text's own) ...
rc:         .res        1                                   ;   and a byte (an item's first)

.bss
text:       .res        TEXT_SIZE + 1                       ; The text (and a 0 after it)
t_len:      .res        2
scratch:    .res        SCRATCH_SIZE
sc_len:     .res        1                                   ; (A message's length, in scratch: 255 at most)
lv_close:   .res        LEVELS + 1                          ; Each level open: its closer (the text's own: 0) ...
lv_head_lo: .res        LEVELS + 1                          ;   its list so far ...
lv_head_hi: .res        LEVELS + 1
lv_tail_lo: .res        LEVELS + 1                          ;   and its last cons
lv_tail_hi: .res        LEVELS + 1
rd_on:      .res        1                                   ; <> 0: a text being read (its levels marked)
closers:    .res        256                                 ; The closers wanted (RD_MORE's, an error's): innermost last
cl_len:     .res        1
rd_sp:      .res        1                                   ; (The stack as read_text began)
rd_v:       .res        2                                   ; (A number's value, a digit, its sign, a digit seen, too
rd_d:       .res        1                                   ;   big; a name's length (char_named's); a word's so far)
rd_neg:     .res        1
rd_dig:     .res        1
rd_big:     .res        1
rd_t:       .res        2
rd_fd:      .res        1                                   ; The source: $FF the text as it is; else a file to refill
rd_eof:     .res        1                                   ;   it from, at its end?, its bytes read so far ...
rd_total:   .res        2
rd_one:     .res        1                                   ;   <> 0: an item at a time (read_item's) ...
rd_inword:  .res        1                                   ;   <> 0: a word being read (from rw: kept as it refills)
rd_owner:   .res        1                                   ; Whose text it is: a load's fd ($FF: the REPL's, or none)
rf_keep:    .res        2                                   ; (rd_fill's: where what's kept begins, the bytes dropped)
rf_drop:    .res        2

.segment "DATA4"
; The levels' lists marked, while a text is read (gc_hook's: hylang.s's)
rd_roots:
            lda         rd_on
            beq         @done
            ldx         #0
@level:
            lda         lv_head_lo,x
            sta         mv
            lda         lv_head_hi,x
            sta         mv + 1
            phx
            jsr         mark_val
            jsr         gc_drain
            plx
            cpx         rd
            beq         @done
            inx
            bra         @level
@done:
            rts

.segment "CODE2"

; The text read: text, t_len bytes and a 0.  OUT: .A = RD_DONE and hv = its S-expression (SNIL: no items); RD_MORE
; and closers = what it wants (cl_len bytes: the open brackets' closers, outermost first, and a here string's quotes);
; or RD_ERROR and hv = the error.  C = 1 and .A = E_NOMEM if there's no room
read_text:
            lda         #$FF                                ; (The text as it is: no file)
            sta         rd_fd
            sta         rd_owner
            stz         rd_one
            lda         #<text
            sta         rp
            clc
            adc         t_len
            sta         re
            lda         #>text
            sta         rp + 1
            adc         t_len + 1
            sta         re + 1
            bra         rd_start

; A file's next item read (load's: rd_fd the file, rp and re where the last one left them, text refilled from it).
; OUT: .A = RD_DONE and hv = the item; RD_END, none left; RD_MORE (closers: what it wants), the file's end in one; or
; RD_ERROR and hv = the error.  C = 1 and .A = E_NOMEM if there's no room
read_item:
            lda         #1
            sta         rd_one
            stz         rd_inword
            jsr         rd_fill
rd_start:
            tsx
            stx         rd_sp
            stz         rd_inword
            stz         rd
            stz         lv_close
            stz         lv_head_lo
            stz         lv_head_hi
            stz         lv_tail_lo
            stz         lv_tail_hi
            lda         #1
            sta         rd_on
@next:
            lda         (rp)
            cmp         #' ' + 1
            bcc         @low
            cmp         #127
            bcs         @high
            cmp         #';'
            beq         @comment
            cmp         #'"'
            beq         @string
            ldx         #5
@bracket:
            cmp         br_char,x
            beq         @br
            dex
            bpl         @bracket
            jsr         rd_item                             ; (A word, or a prefix and its bracket)
            bra         @next
@br:
            cpx         #3
            bcs         @close
            jsr         rd_adv
            lda         br_char + 3,x                       ; (An opener: a level, its closer)
            jsr         rd_open
            cmp         #']'
            bne         @next
            ldx         #SY_LIST                            ; ([a b]: (list a b))
            jsr         rd_append_sym
            bra         @next
@close:
            jsr         rd_close
            bra         @next
@comment:
            jsr         rd_comment
            bra         @next
@string:
            jsr         rd_string
            bra         @next
@low:
            cmp         #' '
            beq         @skip
            cmp         #TAB
            bcc         @nul
            cmp         #CR + 1
            bcc         @skip                               ; (TAB, LF, VT, FF, CR: spaces too)
            bra         @control
@nul:
            cmp         #0
            bne         @control
            jsr         rd_at_end
            bcc         @control                            ; (A 0 in the text: a control character)
            jmp         rd_end
@skip:
            jsr         rd_adv
            bra         @next
@high:
            bne         @ascii                              ; (127 is a control character)
@control:
            jmp         rd_e_control
@ascii:
            jmp         rd_e_ascii

; rp to the next byte (a file's text refilled if less than 2 bytes are left).  Keeps .A, .X, .Y
rd_adv:
            inc         rp
            bne         :+
            inc         rp + 1
:
            bit         rd_fd
            bmi         :+
            jmp         rd_fill
:
            rts

; Less than 2 bytes left of a file's text (and not its end): what's left (from rw, if a word's being read) moved to
; text's start, and the file's next bytes after it.  Keeps .A, .X, .Y
rd_fill:
            pha
            phx
            phy
            lda         rd_eof
            beq         :+
            jmp         @done
:
            lda         re                                  ; (2 bytes left at least: nothing to do)
            sec
            sbc         rp
            tax
            lda         re + 1
            sbc         rp + 1
            beq         :+
            jmp         @done
:
            cpx         #2
            bcc         :+
            jmp         @done
:
            lda         rp                                  ; (What's kept: from rw, or rp)
            ldx         rp + 1
            ldy         rd_inword
            beq         :+
            lda         rw
            ldx         rw + 1
:
            sta         rf_keep
            stx         rf_keep + 1
            sec                                             ; (rf_drop: the bytes before it, dropped)
            sbc         #<text
            sta         rf_drop
            txa
            sbc         #>text
            sta         rf_drop + 1
            ora         rf_drop
            bne         :+
            lda         re + 1                              ; (Nothing to drop, and a full text: a word too long)
            cmp         #>(text + TEXT_SIZE)
            bcc         :+
            jmp         rd_e_wordlong
:
            lda         hq                                  ; (The kept bytes moved down: hq from, hb to, kept)
            pha
            lda         hq + 1
            pha
            lda         hb
            pha
            lda         hb + 1
            pha
            MOVW        hq, rf_keep
            lda         #<text
            sta         hb
            lda         #>text
            sta         hb + 1
@move:
            lda         hq
            cmp         re
            lda         hq + 1
            sbc         re + 1
            bcs         @put
            lda         (hq)
            sta         (hb)
            inc         hq
            bne         :+
            inc         hq + 1
:
            inc         hb
            bne         @move
            inc         hb + 1
            bra         @move
@put:
            pla
            sta         hb + 1
            pla
            sta         hb
            pla
            sta         hq + 1
            pla
            sta         hq
@moved:
            sec                                             ; (rp, rw, re: less the bytes dropped)
            lda         rp
            sbc         rf_drop
            sta         rp
            lda         rp + 1
            sbc         rf_drop + 1
            sta         rp + 1
            sec
            lda         rw
            sbc         rf_drop
            sta         rw
            lda         rw + 1
            sbc         rf_drop + 1
            sta         rw + 1
            sec
            lda         re
            sbc         rf_drop
            sta         re
            sta         r0
            lda         re + 1
            sbc         rf_drop + 1
            sta         re + 1
            sta         r0 + 1
            lda         #<(text + TEXT_SIZE)                ; (The file's next bytes: as many as there's room for)
            sec
            sbc         re
            sta         r1
            lda         #>(text + TEXT_SIZE)
            sbc         re + 1
            sta         r1 + 1
            lda         rd_fd
            jsr         READ
            bcs         @end
            sta         rf_drop
            stx         rf_drop + 1
            ora         rf_drop + 1
            beq         @end
            clc
            lda         re
            adc         rf_drop
            sta         re
            lda         re + 1
            adc         rf_drop + 1
            sta         re + 1
            clc
            lda         rd_total
            adc         rf_drop
            sta         rd_total
            lda         rd_total + 1
            adc         rf_drop + 1
            sta         rd_total + 1
            bra         @zero
@end:
            lda         #1                                  ; (Its end, or an error: no more)
            sta         rd_eof
@zero:
            lda         #0                                  ; (The text's end: a 0)
            sta         (re)
@done:
            ply
            plx
            pla
            rts

; OUT: C = 1 if rp is at the text's end (a file's: rd_fill has refilled it, if there was more)
rd_at_end:
            lda         rp
            cmp         re
            lda         rp + 1
            sbc         re + 1
            rts

; ---- Levels and lists

; A level opened: .A = its closer (kept)
rd_open:
            ldx         rd
            cpx         #LEVELS
            bcs         @deep
            inx
            sta         lv_close,x
            stz         lv_head_lo,x
            stz         lv_head_hi,x
            stz         lv_tail_lo,x
            stz         lv_tail_hi,x
            stx         rd
            rts
@deep:
            jmp         rd_e_deep

; A closer (.A, at rp): its level's list put in the level it's in.  Not the level's closer (or no level open): an
; error
rd_close:
            ldx         rd
            beq         @wrong
            cmp         lv_close,x
            bne         @wrong
            jsr         rd_adv
            jmp         rd_pop
@wrong:
            sta         rc
            jmp         rd_e_closed

; The level closed: its list (or, empty, () SNIL and {} NIL) put at the end of the one it's in
rd_pop:
            ldx         rd
            lda         lv_head_lo,x
            sta         VR0
            lda         lv_head_hi,x
            sta         VR0 + 1
            ora         VR0
            bne         @have
            lda         lv_close,x
            cmp         #'}'
            beq         @have
            lda         #<SNIL
            sta         VR0
            lda         #>SNIL
            sta         VR0 + 1
@have:
            dec         rd
            jmp         rd_append

; The symbol .X (SY_*) put at the end of the level's list
rd_append_sym:
            lda         sy_lo,x
            sta         hq
            lda         sy_hi,x
            sta         hq + 1
            lda         sy_len,x
            sta         hn
            stz         hn + 1
            lda         #PK_SYMBOL
; The symbol (.A = PK_SYMBOL) or atom (PK_ATOM) named hq, hn bytes put at the end of the level's list
rd_append_name:
            jsr         intern
            bcs         rd_nomem
; hv put at the end of the level's list
rd_append_hv:
            lda         hv
            sta         VR0
            lda         hv + 1
            sta         VR0 + 1
; VR0 put at the end of the level's list: a cons of its kind (a Q-expression's or an S-expression's).  At level
; 0, an item at a time (read_item's): the item is read_item's value
rd_append:
            ldx         rd
            bne         :+
            lda         rd_one
            beq         :+
            MOVW        hv, VR0
            lda         #RD_DONE
            jmp         rd_return
:
            ldx         rd
            lda         #PK_SCONS
            ldy         lv_close,x
            cpy         #'}'
            bne         :+
            lda         #PK_QCONS
:
            jsr         cell_alloc
            bcs         rd_nomem
            lda         VR0                                 ; (Its car: the item)
            sta         (hp)
            ldy         #1
            lda         VR0 + 1
            sta         (hp),y
            ldx         rd
            lda         lv_tail_lo,x
            ora         lv_tail_hi,x
            bne         @link
            lda         hv                                  ; (The first: the list)
            sta         lv_head_lo,x
            lda         hv + 1
            sta         lv_head_hi,x
            bra         @tail
@link:
            lda         lv_tail_lo,x                        ; (After the last: its cdr)
            pha
            lda         lv_tail_hi,x
            tax
            pla
            jsr         deref
            ldy         #2
            lda         hv
            sta         (hp),y
            iny
            lda         hv + 1
            sta         (hp),y
            ldx         rd
@tail:
            lda         hv
            sta         lv_tail_lo,x
            lda         hv + 1
            sta         lv_tail_hi,x
            rts

; No room: C = 1, .A = E_NOMEM, to read_text's caller
rd_nomem:
            ldx         rd_sp
            txs
            stz         rd_on
            lda         #E_NOMEM
            sec
            rts

; The text's end: its list read, or more wanted (rd_more: rq the quotes of a here string open, or 0)
rd_end:
            stz         rq
            lda         rd
            bne         rd_more
            lda         rd_one                              ; (An item at a time: none left)
            beq         :+
            lda         #RD_END
            bra         rd_return
:
            lda         lv_head_lo
            sta         hv
            lda         lv_head_hi
            sta         hv + 1
            ora         hv
            bne         :+
            lda         #<SNIL
            sta         hv
            lda         #>SNIL
            sta         hv + 1
:
            lda         #RD_DONE
; .A to read_text's caller
rd_return:
            ldx         rd_sp
            txs
            stz         rd_on
            clc
            rts

rd_more:
            jsr         rd_closers
            ldy         rq
            beq         @done
@quote:
            cpx         #255
            bcs         @done
            lda         #'"'
            sta         closers,x
            inx
            dey
            bne         @quote
@done:
            stx         cl_len
            lda         #RD_MORE
            bra         rd_return

; closers: the open levels' closers, outermost first.  OUT: .X = cl_len, how many
rd_closers:
            ldx         #0
@level:
            cpx         rd
            bcs         @done
            lda         lv_close + 1,x
            sta         closers,x
            inx
            bra         @level
@done:
            stx         cl_len
            rts

; ---- Comments, strings

; A comment, from ; to the line's end
rd_comment:
            jsr         rd_adv
            lda         (rp)
            beq         @nul
            cmp         #LF
            beq         @done
            cmp         #CR
            bne         rd_comment
@done:
            rts
@nul:
            jsr         rd_at_end
            bcc         rd_comment                          ; (A 0 in a comment: its)
            rts

; A string ("text", with escapes) or a here string ("""text""": as many quotes close it as open it, its text as it
; is, a CR LF in it an LF)
rd_string:
            stz         rq
@open:
            jsr         rd_adv
            inc         rq
            beq         @many
            lda         (rp)
            cmp         #'"'
            beq         @open
            lda         #<scratch
            sta         rs
            lda         #>scratch
            sta         rs + 1
            lda         rq
            lsr
            bcs         @byte
            jmp         @made                               ; (Quotes in pairs: the empty string)
@many:
            jmp         rd_e_quotes
@byte:
            lda         (rp)
            cmp         #' '
            bcc         @low
            cmp         #127
            beq         @control
            cmp         #'"'
            bne         :+
            jmp         @quote
:
            cmp         #'\'
            bne         @keep
            ldx         rq
            dex
            beq         @escape                             ; (A here string's: as it is)
@keep:
            jsr         rd_adv
@store:
            sta         (rs)
            inc         rs
            bne         @byte
            inc         rs + 1
            bra         @byte
@low:
            cmp         #LF
            beq         @lf
            cmp         #CR
            beq         @cr
            cmp         #0
            beq         @nul
            cmp         #TAB
            bcc         @control
            cmp         #CR
            bcc         @keep                               ; (TAB, VT, FF)
@control:
            jmp         rd_e_control
@lf:
            ldx         rq
            dex
            bne         @keep
@newline:
            jmp         rd_e_newline
@cr:
            ldx         rq
            dex
            beq         @newline
            ldy         #1
            lda         (rp),y
            cmp         #LF
            beq         @crlf
            lda         #CR
            bra         @keep
@crlf:
            jsr         rd_adv                              ; (CR LF: the LF)
            bra         @byte
@nul:
            jsr         rd_at_end
            bcc         @control
            lda         rq
            cmp         #1
            beq         @incomplete
            jmp         rd_more                             ; (A here string open at the end: more wanted)
@incomplete:
            jmp         rd_e_incomplete
@escape:
            jsr         rd_adv
            lda         (rp)
            bne         @letter
            jsr         rd_at_end
            bcs         @incomplete
            lda         #0                                  ; (A 0: no escape's)
@letter:
            ldx         #ESCAPES - 1
@find:
            cmp         esc_from,x
            beq         @found
            dex
            bpl         @find
            cmp         #'x'
            beq         @hex
            jmp         rd_e_escape
@found:
            lda         esc_to,x
            bra         @keep
@hex:
            jsr         rd_adv                              ; (\x and one or two hex digits)
            jsr         rd_hex
            bcs         @nohex
            sta         rc
            jsr         rd_hex
            bcs         @one
            pha
            lda         rc
            asl
            asl
            asl
            asl
            sta         rc
            pla
            ora         rc
            jmp         @store
@one:
            lda         rc
            jmp         @store
@nohex:
            jmp         rd_e_hex
@quote:
            jsr         rd_adv                              ; (A quote, and those after it: as many as opened
            lda         #1                                  ;   it, its end; fewer, the string's)
            sta         rc
@trail:
            lda         rc
            cmp         rq
            beq         @made
            lda         (rp)
            cmp         #'"'
            bne         @short
            jsr         rd_adv
            inc         rc
            bra         @trail
@short:
            ldx         rc
            lda         #'"'
@q:
            sta         (rs)
            inc         rs
            bne         :+
            inc         rs + 1
:
            dex
            bne         @q
            jmp         @byte
@made:
            lda         #<scratch
            sta         hq
            lda         #>scratch
            sta         hq + 1
            lda         rs
            sec
            sbc         #<scratch
            sta         hn
            lda         rs + 1
            sbc         #>scratch
            sta         hn + 1
            jsr         string_make
            bcc         :+
            jmp         rd_nomem
:
            jmp         rd_append_hv

; A hex digit at rp.  OUT: C = 0, .A = its value, rp past it; or C = 1: none
rd_hex:
            lda         (rp)
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         @digit
            ora         #$20                                ; (A-F as a-f)
            cmp         #'a'
            bcc         @no
            cmp         #'f' + 1
            bcs         @no
            sbc         #'a' - 11                           ; (C = 0: less 'a', and 10)
            bra         @take
@digit:
            sbc         #'0' - 1                            ; (C = 0: less '0')
@take:
            jsr         rd_adv
            clc
            rts
@no:
            sec
            rts

; ---- Words

; An item that starts with a word's byte (.A, at rp): a prefix right before ( or { (its bracket opened, its function
; first), or a word (to a space, a bracket or the end): a character, $name, an atom, a number or a symbol
rd_item:
            sta         rc
            ldy         #1
            lda         (rp),y
            cmp         #'('
            beq         @opener
            cmp         #'{'
            beq         @opener
            cmp         #'['
            bne         @word
            lda         rc
            cmp         #'$'
            bne         @word
            jmp         rd_e_dollar
@opener:
            lda         rc
            ldx         #PREFIXES - 1
@prefix:
            cmp         sy_char,x
            beq         @short
            dex
            bpl         @prefix
            cmp         #'$'
            bne         @word
            jmp         rd_e_dollar
@short:
            jsr         rd_adv
            lda         (rp)
            jsr         rd_adv
            ldy         #')'
            cmp         #'('
            beq         :+
            ldy         #'}'
:
            tya
            phx
            jsr         rd_open
            plx
            jmp         rd_append_sym
@word:
            lda         rp
            sta         rw
            lda         rp + 1
            sta         rw + 1
            lda         #1                                  ; (Kept, as text refills, till it's read)
            sta         rd_inword
            jsr         rd_scan
            stz         rd_inword
            sec
            lda         rp
            sbc         rw
            sta         rn
            lda         rp + 1
            sbc         rw + 1
            sta         rn + 1
            lda         rc
            cmp         #'\'
            bne         :+
            jmp         rd_char
:
            cmp         #':'
            bne         :+
            jmp         rd_atom
:
            cmp         #'$'
            bne         :+
            jmp         rd_env
:
            cmp         #'#'
            beq         @number
            cmp         #'+'
            beq         @number
            cmp         #'-'
            beq         @number
            cmp         #'0'
            bcc         @symbol
            cmp         #'9' + 1
            bcs         @symbol
@number:
            jmp         rd_number
@symbol:
            jmp         rd_symbol

; rp past the word at rp: to a space, a bracket or the end (with a digit set of its own, #[...], in it).  A control
; character or a byte past ASCII in it: an error
rd_scan:
            lda         (rp)
            cmp         #' ' + 1
            bcc         @low
            cmp         #127
            bcs         @high
            cmp         #'('
            beq         @end
            cmp         #')'
            beq         @end
            cmp         #'{'
            beq         @end
            cmp         #'}'
            beq         @end
            cmp         #']'
            beq         @end
            cmp         #'['
            beq         @set
@next:
            jsr         rd_adv
            bra         rd_scan
@low:
            cmp         #' '
            beq         @end
            cmp         #TAB
            bcc         @nul
            cmp         #CR + 1
            bcc         @end
@control:
            jmp         rd_e_control
@nul:
            cmp         #0
            bne         @control
            jsr         rd_at_end
            bcc         @control
@end:
            rts
@high:
            beq         @control
@ascii:
            jmp         rd_e_ascii
@set:
            jsr         rd_own_digits
            bcs         @end
@inset:
            jsr         rd_adv                              ; (To its ], and past it)
            lda         (rp)
            cmp         #']'
            beq         @next
            cmp         #' '
            bcc         @setlow
            cmp         #127
            bcc         @inset
            beq         @control
            bra         @ascii
@setlow:
            cmp         #0
            bne         @control
            jsr         rd_at_end
            bcc         @control
            rts

; OUT: C = 0 if the word so far (rw to rp) is [+-]?#[<>]?=?[+-]?, as before a digit set of its own
rd_own_digits:
            sec
            lda         rp
            sbc         rw
            sta         rd_t
            lda         rp + 1
            sbc         rw + 1
            bne         @no
            ldy         #0
            lda         (rw),y
            jsr         @sign
            lda         (rw),y
            cmp         #'#'
            bne         @no
            iny
            lda         (rw),y
            cmp         #'<'
            beq         @angle
            cmp         #'>'
            bne         @equal
@angle:
            iny
@equal:
            lda         (rw),y
            cmp         #'='
            bne         @last
            iny
@last:
            lda         (rw),y
            jsr         @sign
            cpy         rd_t
            bne         @no
            clc
            rts
@no:
            sec
            rts
@sign:
            cmp         #'+'
            beq         @signed
            cmp         #'-'
            bne         @unsigned
@signed:
            iny
@unsigned:
            rts

; After a word: an opener right against it is an error (f(x), x[1] ...)
rd_glued:
            lda         (rp)
            cmp         #'('
            beq         @touch
            cmp         #'{'
            beq         @touch
            cmp         #'['
            beq         @touch
            rts
@touch:
            jmp         rd_e_touch

; A character: \ and a bracket (the bracket), one byte (it), or a name (char_named's)
rd_char:
            lda         rn + 1
            bne         @unknown
            lda         rn
            cmp         #1
            bne         @named
            lda         (rp)                                ; (\ and a bracket: the bracket, glued or not)
            ldx         #5
@bracket:
            cmp         br_char,x
            beq         @br
            dex
            bpl         @bracket
            jmp         rd_e_noname
@br:
            jsr         rd_adv
            bra         @char
@named:
            cmp         #2
            bne         @name
            ldy         #1
            lda         (rw),y
            bra         @glue
@name:
            jsr         char_named
            bcs         @unknown
@glue:
            pha
            jsr         rd_glued
            pla
@char:
            stz         VR0 + 1                             ; (CHAR0 + 2 * its code)
            asl
            sta         VR0
            rol         VR0 + 1
            lda         VR0 + 1
            ora         #>CHAR0
            sta         VR0 + 1
            jmp         rd_append
@unknown:
            jmp         rd_e_charname

; The character the word's name names (after its \: in any case, its -s ignored).  OUT: C = 0, .A = its code; C = 1
; if it isn't one
char_named:
            ldx         #0
            ldy         #1
@byte:
            cpy         rn
            beq         @look
            lda         (rw),y
            iny
            cmp         #'-'
            beq         @byte
            cmp         #'A'
            bcc         :+
            cmp         #'Z' + 1
            bcs         :+
            ora         #$20
:
            cpx         #32
            bcs         @no
            sta         scratch,x
            inx
            bra         @byte
@look:
            stx         rd_d
            lda         #<cn_table
            sta         hq
            lda         #>cn_table
            sta         hq + 1
@entry:
            lda         (hq)
            beq         @no
            cmp         rd_d
            bne         @skip
            ldy         #2
            ldx         #0
@same:
            lda         (hq),y
            cmp         scratch,x
            bne         @skip
            iny
            inx
            cpx         rd_d
            bne         @same
            ldy         #1
            lda         (hq),y
            and         #$7F
            clc
            rts
@skip:
            lda         (hq)
            sec                                                 ; (Its length, and 2)
            adc         #1
            adc         hq
            sta         hq
            bcc         @entry
            inc         hq + 1
            bra         @entry
@no:
            sec
            rts

; $name: (env "name"), its name's case kept ($ alone: a symbol)
rd_env:
            lda         rn + 1
            bne         @env
            lda         rn
            cmp         #1
            bne         @env
            jmp         rd_symbol
@env:
            jsr         rd_glued
            lda         #')'
            jsr         rd_open
            ldx         #SY_ENV
            jsr         rd_append_sym
            lda         rw                                  ; (The name, a string)
            clc
            adc         #1
            sta         hq
            lda         rw + 1
            adc         #0
            sta         hq + 1
            lda         rn
            sec
            sbc         #1
            sta         hn
            lda         rn + 1
            sbc         #0
            sta         hn + 1
            jsr         string_make
            bcc         :+
            jmp         rd_nomem
:
            jsr         rd_append_hv
            jmp         rd_pop

; An atom, :name (its name in lower case)
rd_atom:
            jsr         rd_glued
            jsr         rd_lower1
            lda         #PK_ATOM
            jmp         rd_append_name

; A word that starts like a number: as yet, a decimal integer ([+-]?[0-9_]+, a digit in it) that a fixnum holds (one
; past it: an error till phase 5); any other, a symbol (phase 5: the reference's numbers)
rd_number:
            lda         rn + 1
            beq         :+
            jmp         rd_symbol
:
            stz         rd_v
            stz         rd_v + 1
            stz         rd_neg
            stz         rd_dig
            stz         rd_big
            ldy         #0
            lda         (rw)
            cmp         #'+'
            beq         @sign
            cmp         #'-'
            bne         @digits
            inc         rd_neg
@sign:
            iny
@digits:
            cpy         rn
            bne         :+
            jmp         @end
:
            lda         (rw),y
            iny
            cmp         #'_'
            beq         @digits
            sec
            sbc         #'0'
            cmp         #10
            bcc         :+
            jmp         rd_symbol
:
            sta         rd_d
            sta         rd_dig
            inc         rd_dig                              ; (A digit seen: <> 0)
            lda         rd_big
            bne         @digits
            lda         rd_v + 1                            ; (Past 16384 after this digit: too big)
            cmp         #>1639
            bcc         @times
            bne         @big
            lda         rd_v
            cmp         #<1639
            bcs         @big
@times:
            asl         rd_v                                ; (Times 10, and the digit)
            rol         rd_v + 1
            lda         rd_v
            sta         rd_t
            lda         rd_v + 1
            sta         rd_t + 1
            asl         rd_v
            rol         rd_v + 1
            asl         rd_v
            rol         rd_v + 1
            clc
            lda         rd_v
            adc         rd_t
            sta         rd_v
            lda         rd_v + 1
            adc         rd_t + 1
            sta         rd_v + 1
            clc
            lda         rd_v
            adc         rd_d
            sta         rd_v
            bcc         :+
            inc         rd_v + 1
:
            lda         rd_v + 1
            cmp         #>16384
            bcc         @more
            bne         @big
            lda         rd_v
            bne         @big
@more:
            jmp         @digits
@big:
            lda         #1
            sta         rd_big
            jmp         @digits
@end:
            lda         rd_dig
            beq         @symbol
            lda         rd_big
            bne         @bignum
            lda         rd_neg
            beq         @make
            sec
            lda         #0
            sbc         rd_v
            sta         rd_v
            lda         #0
            sbc         rd_v + 1
            sta         rd_v + 1
@make:
            lda         rd_v
            ldx         rd_v + 1
            jsr         fix_make
            bcs         @bignum
            jsr         rd_glued
            jmp         rd_append_hv
@bignum:
            jmp         rd_e_bignum
@symbol:
            jmp         rd_symbol

; A symbol (its name in lower case): T, NIL and exit are theirs
rd_symbol:
            jsr         rd_glued
            jsr         rd_lower
            lda         hn
            cmp         #1
            bne         @nil
            lda         scratch
            cmp         #'t'
            bne         @name
            lda         #<T_VAL
            ldx         #>T_VAL
            bra         @value
@nil:
            cmp         #3
            bne         @exit
            lda         scratch
            cmp         #'n'
            bne         @name
            lda         scratch + 1
            cmp         #'i'
            bne         @name
            lda         scratch + 2
            cmp         #'l'
            bne         @name
            lda         #<NIL
            ldx         #>NIL
            bra         @value
@exit:
            cmp         #4
            bne         @name
            ldx         #3
@e:
            lda         scratch,x
            cmp         s_exit,x
            bne         @name
            dex
            bpl         @e
            lda         #<EXIT_VAL
            ldx         #>EXIT_VAL
@value:
            sta         VR0
            stx         VR0 + 1
            jmp         rd_append
@name:
            lda         #PK_SYMBOL
            jmp         rd_append_name

; The word, from its first byte (rd_lower) or its second (rd_lower1), in lower case in scratch: hq, hn it.  Longer
; than 255 bytes: an error
rd_lower1:
            ldy         #1
            bra         rd_low
rd_lower:
            ldy         #0
rd_low:
            sty         rd_d
            sec
            lda         rn
            sbc         rd_d
            sta         hn
            lda         rn + 1
            sbc         #0
            bne         @long
            stz         hn + 1
            ldx         #0
@byte:
            cpx         hn
            beq         @done
            lda         (rw),y
            cmp         #'A'
            bcc         :+
            cmp         #'Z' + 1
            bcs         :+
            ora         #$20
:
            sta         scratch,x
            iny
            inx
            bra         @byte
@done:
            lda         #<scratch
            sta         hq
            lda         #>scratch
            sta         hq + 1
            rts
@long:
            jmp         rd_e_long

; ---- Errors: the message in scratch, made an error, read_text's value

rd_e_control:
            ldx         #M_CONTROL
            bra         rd_fail_m
rd_e_ascii:
            ldx         #M_ASCII
            bra         rd_fail_m
rd_e_newline:
            ldx         #M_NEWLINE
            bra         rd_fail_m
rd_e_incomplete:
            ldx         #M_INCOMPLETE
            bra         rd_fail_m
rd_e_hex:
            ldx         #M_HEX
            bra         rd_fail_m
rd_e_noname:
            ldx         #M_NONAME
            bra         rd_fail_m
rd_e_deep:
            ldx         #M_DEEP
            bra         rd_fail_m
rd_e_long:
            ldx         #M_LONG
            bra         rd_fail_m
rd_e_quotes:
            ldx         #M_QUOTES
            bra         rd_fail_m
rd_e_wordlong:
            ldx         #M_WORDLONG
            bra         rd_fail_m
rd_e_bignum:
            ldx         #M_BIGNUM
rd_fail_m:
            jsr         msg_n
; The message (scratch, sc_len bytes) an error: read_text's value
rd_fail:
            lda         #<scratch
            sta         hq
            lda         #>scratch
            sta         hq + 1
            lda         sc_len
            sta         hn
            stz         hn + 1
            jsr         error_make
            bcc         :+
            jmp         rd_nomem
:
            lda         #RD_ERROR
            jmp         rd_return

rd_e_escape:                                                ; (rp at the byte after \)
            ldx         #M_ESCAPE
            jsr         msg_n
            lda         (rp)
            jsr         msg_byte
            bra         rd_fail
rd_e_charname:
            ldx         #M_CHARNAME
            jsr         msg_n
            jsr         msg_word
            bra         rd_fail
rd_e_touch:                                                 ; (rp at the opener)
            ldx         #M_QUOTE
            jsr         msg_n
            jsr         msg_word
            ldx         #M_TOUCHES
            jsr         msg_add
            lda         (rp)
            jsr         msg_byte
            ldx         #M_SPACE
            jsr         msg_add
            bra         rd_fail
rd_e_dollar:                                                ; (rp at the $, the opener after it)
            ldx         #M_DOLLAR
            jsr         msg_n
            ldy         #1
            lda         (rp),y
            jsr         msg_byte
            ldx         #M_ENV
            jsr         msg_add
            bra         rd_fail
rd_e_closed:                                                ; (rc the closer)
            ldx         #M_SEXPR
            lda         rc
            cmp         #')'
            beq         :+
            ldx         #M_QEXPR
            cmp         #'}'
            beq         :+
            ldx         #M_LIST
:
            jsr         msg_n
            jsr         rd_closers
            ldx         #0
@closer:
            cpx         cl_len
            bcs         @done
            lda         closers,x
            jsr         msg_byte
            inx
            bra         @closer
@done:
            jmp         rd_fail

; The message: message .X alone
msg_n:
            stz         sc_len
; Message .X added to the message
msg_add:
            lda         m_lo,x
            sta         hq
            lda         m_hi,x
            sta         hq + 1
            ldy         #0
@byte:
            lda         (hq),y
            beq         @done
            jsr         msg_byte
            iny
            bne         @byte
@done:
            rts

; The word (rw, rn bytes) added to the message
msg_word:
            ldy         #0
@byte:
            lda         rn + 1
            bne         :+
            cpy         rn
            beq         @done
:
            lda         (rw),y
            jsr         msg_byte
            iny
            bne         @byte
@done:
            rts

; .A added to the message (255 bytes at most).  Keeps .X, .Y
msg_byte:
            phy
            ldy         sc_len
            cpy         #255
            bcs         :+
            sta         scratch,y
            inc         sc_len
:
            ply
            rts

.segment "RODATA2"

br_char:    .byte       "({[)}]"                            ; (The brackets: openers, then their closers)

; The symbols the reader puts in: the prefixes' functions (in sy_char's order), list ([a b]) and env ($name)
PREFIXES        = 7
SY_LIST         = 7
SY_ENV          = 8
sy_char:    .byte       "?=:#@.~"
sy_lo:      .lobytes    n_if, n_set, n_def, n_hash, n_fn, n_unpack, n_format, n_list, n_env
sy_hi:      .hibytes    n_if, n_set, n_def, n_hash, n_fn, n_unpack, n_format, n_list, n_env
sy_len:     .byte       2, 3, 3, 11, 2, 6, 6, 4, 3
n_if:       .byte       "if"
n_set:      .byte       "set"
n_def:      .byte       "def"
n_hash:     .byte       "hash-create"
n_fn:       .byte       "fn"
n_unpack:   .byte       "unpack"
n_format:   .byte       "format"
n_list:     .byte       "list"
n_env:      .byte       "env"
s_exit:     .byte       "exit"

; A string's escapes: \n \t \r \0 \a \b \f \v \e \\ \" (and \xHH)
ESCAPES         = 11
esc_from:   .byte       "ntr0abfve", '\', '"'
esc_to:     .byte       LF, TAB, CR, 0, 7, 8, 12, 11, 27, '\', '"'

; The characters' names (danlang's LVal.CharOf and CharName): each its length, its code (CN_PRINT in it: the name
; it prints by), its name in lower case without -s
CN_PRINT        = $80
.macro CN name, code
            .byte       .strlen(name), code, name
.endmacro
cn_table:
            CN          "backslash", '\' | CN_PRINT
            CN          "bslash", '\'
            CN          "bs", '\'
            CN          "backtick", '`' | CN_PRINT
            CN          "btick", '`'
            CN          "bt", '`'
            CN          "bell", 7 | CN_PRINT
            CN          "backspace", 8 | CN_PRINT
            CN          "bksp", 8
            CN          "linefeed", LF
            CN          "newline", LF
            CN          "lf", LF | CN_PRINT
            CN          "nl", LF
            CN          "formfeed", 12
            CN          "ff", 12 | CN_PRINT
            CN          "null", 0 | CN_PRINT
            CN          "quote", '"' | CN_PRINT
            CN          "doublequote", '"'
            CN          "return", CR
            CN          "carriagereturn", CR
            CN          "cr", CR | CN_PRINT
            CN          "rparen", ')' | CN_PRINT
            CN          "rightparen", ')'
            CN          "rp", ')'
            CN          "lparen", '(' | CN_PRINT
            CN          "leftparen", '('
            CN          "lp", '('
            CN          "rbrace", '}'
            CN          "rightcurly", '}'
            CN          "rcurly", '}' | CN_PRINT
            CN          "rc", '}'
            CN          "lbrace", '{'
            CN          "leftcurly", '{'
            CN          "lcurly", '{' | CN_PRINT
            CN          "lc", '{'
            CN          "rbracket", ']' | CN_PRINT
            CN          "rightbracket", ']'
            CN          "rb", ']'
            CN          "lbracket", '[' | CN_PRINT
            CN          "leftbracket", '['
            CN          "lb", '['
            CN          "slash", '/' | CN_PRINT
            CN          "sl", '/'
            CN          "space", ' ' | CN_PRINT
            CN          "sp", ' '
            CN          "tab", TAB | CN_PRINT
            CN          "tick", $27 | CN_PRINT
            CN          "singlequote", $27
            CN          "tilde", '~' | CN_PRINT
            CN          "verticaltab", 11
            CN          "vtab", 11 | CN_PRINT
            CN          "vt", 11
            CN          "verticalspace", 11
            CN          "vspace", 11
            CN          "vs", 11
            CN          "bang", '!' | CN_PRINT
            CN          "warning", '!'
            CN          "exclaim", '!'
            CN          "exclamation", '!'
            CN          "exclamationpoint", '!'
            CN          "at", '@' | CN_PRINT
            CN          "poundsign", '#'
            CN          "pound", '#'
            CN          "hash", '#' | CN_PRINT
            CN          "dollarsign", '$'
            CN          "dollar", '$' | CN_PRINT
            CN          "dollars", '$'
            CN          "ds", '$'
            CN          "percentsign", '%'
            CN          "percent", '%' | CN_PRINT
            CN          "mod", '%'
            CN          "modulo", '%'
            CN          "modulus", '%'
            CN          "pc", '%'
            CN          "caret", '^' | CN_PRINT
            CN          "uparrow", '^'
            CN          "amp", '&' | CN_PRINT
            CN          "and", '&'
            CN          "ampersand", '&'
            CN          "pipe", '|' | CN_PRINT
            CN          "or", '|'
            CN          "vbar", '|'
            CN          "verticalbar", '|'
            CN          "vb", '|'
            CN          "star", '*' | CN_PRINT
            CN          "splat", '*'
            CN          "mult", '*'
            CN          "st", '*'
            CN          "gt", '>' | CN_PRINT
            CN          "greater", '>'
            CN          "greaterthan", '>'
            CN          "lt", '<' | CN_PRINT
            CN          "less", '<'
            CN          "lessthan", '<'
            CN          "comma", ',' | CN_PRINT
            CN          "colon", ':' | CN_PRINT
            CN          "semicolon", ';' | CN_PRINT
            CN          "semi", ';'
            CN          "sc", ';'
            CN          "dot", '.' | CN_PRINT
            CN          "period", '.'
            CN          "point", '.'
            CN          "qmark", '?' | CN_PRINT
            CN          "question", '?'
            CN          "questionmark", '?'
            CN          "qm", '?'
            CN          "underbar", '_' | CN_PRINT
            CN          "ub", '_'
            CN          "underscore", '_'
            CN          "minus", '-' | CN_PRINT
            CN          "hyphen", '-'
            CN          "dash", '-'
            CN          "sub", '-'
            CN          "subtract", '-'
            CN          "plus", '+' | CN_PRINT
            CN          "add", '+'
            CN          "escape", 27 | CN_PRINT
            CN          "esc", 27
            CN          "delete", 127 | CN_PRINT
            CN          "del", 127
            .byte       0

; The reader's messages (danlang's, but for those of hylang's limits)
M_CONTROL       = 0
M_ASCII         = 1
M_NEWLINE       = 2
M_INCOMPLETE    = 3
M_ESCAPE        = 4
M_HEX           = 5
M_NONAME        = 6
M_CHARNAME      = 7
M_QUOTE         = 8
M_TOUCHES       = 9
M_SPACE         = 10
M_DOLLAR        = 11
M_ENV           = 12
M_LIST          = 13
M_SEXPR         = 14
M_QEXPR         = 15
M_DEEP          = 16
M_LONG          = 17
M_QUOTES        = 18
M_BIGNUM        = 19
M_WORDLONG      = 20
m_lo:       .lobytes    m_control, m_ascii, m_newline, m_incomplete, m_escape, m_hex, m_noname, m_charname
            .lobytes    m_quote, m_touches, m_space, m_dollar, m_env, m_list, m_sexpr, m_qexpr, m_deep, m_long
            .lobytes    m_quotes, m_bignum, m_wordlong
m_hi:       .hibytes    m_control, m_ascii, m_newline, m_incomplete, m_escape, m_hex, m_noname, m_charname
            .hibytes    m_quote, m_touches, m_space, m_dollar, m_env, m_list, m_sexpr, m_qexpr, m_deep, m_long
            .hibytes    m_quotes, m_bignum, m_wordlong
m_control:  .byte       "Control sequences not allowed", 0
m_ascii:    .byte       "Non-ASCII character outside of string literal", 0
m_newline:  .byte       "Newlines are not allowed in regular strings", 0
m_incomplete: .byte     "Incomplete string literal", 0
m_escape:   .byte       "Unknown escape sequence ", '\', 0
m_hex:      .byte       '\', "x needs a hex digit", 0
m_noname:   .byte       "A character needs a name", 0
m_charname: .byte       "Unknown character name ", 0
m_quote:    .byte       $27, 0
m_touches:  .byte       $27, " touches ", $27, 0
m_space:    .byte       $27, ": put a space between them", 0
m_dollar:   .byte       $27, "$", 0
m_env:      .byte       $27, " isn", $27, "t danlang: $name is the environment", $27, "s variable", 0
m_list:     .byte       "Closed a list without opening: ", 0
m_sexpr:    .byte       "Closed a SExpr without opening: ", 0
m_qexpr:    .byte       "Closed a QExpr without opening: ", 0
m_deep:     .byte       "Too deep: more than 255 brackets open", 0
m_long:     .byte       "Too long: a name of more than 255 bytes", 0
m_quotes:   .byte       "Too long: a here string", $27, "s quotes, more than 255", 0
m_bignum:   .byte       "Not yet: an integer past a fixnum (phase 5)", 0
m_wordlong: .byte       "Too long: a word of more than 4096 bytes", 0
