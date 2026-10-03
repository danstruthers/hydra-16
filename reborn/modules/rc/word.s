; ****************************************************************************
; word.s - words expanded into lists (in the arena), globbing, and patterns.
;   expand:       a word node -> a list (glob markers still in its text's words)
;   expand_list:  a word list -> one list, globbed (a word with markers: the names it matches, sorted; none: the
;                 word itself, its markers made characters again)
;   expand_one:   a word -> a list, its markers made characters (a subject, a name, a redirection's file)
;   match:        a pattern (markers: * ? [...]) against a string
; Concatenation (^): two lists of the same length pair up; one of one word joins each of the other's; anything else
; is an error.  `{cmd}'s output is split at spaces, tabs and new lines.

.include "rc.inc"

.zeropage
wn:         .res        2                                   ; A node
wa:         .res        2                                   ; A list ...
wb:         .res        2                                   ;   another
wc:         .res        2                                   ; A chain's first cell ...
wd:         .res        2                                   ;   and its last
we:         .res        2                                   ; A word in a list
ws:         .res        2                                   ; (sort's: the least so far)
mp_p:       .res        2                                   ; match's: the pattern ...
mp_s:       .res        2                                   ;   and the string

.bss
wl:         .res        1                                   ; A length
mpl:        .res        1                                   ;   their lengths
msl:        .res        1
mi:         .res        1                                   ;   where in each
mj:         .res        1
mstar:      .res        1                                   ;   the last * (its place, + 1; 0: none) ...
mstars:     .res        1                                   ;   and the string's place then
wk:         .res        1                                   ; (nth's count)
kc:         .res        1                                   ; klass's: the character ...
kn:         .res        1                                   ;   1: ~ (not) ...
kf:         .res        1                                   ;   found ...
klo:        .res        1                                   ;   and a range's ends
khi:        .res        1
wbuf:       .res        WORD_MAX + 1                        ; A word being made

.code

; ****************************************************************************
; The word node at .A/.X expanded.  OUT: .A/.X = its list
expand:
            sta         wn
            stx         wn + 1
            lda         (wn)
            asl
            tax
            jmp         (exp_vec - 2,X)

; W_LIT: its text, a word
x_lit:
            ldy         #1
            lda         (wn),Y
            pha
            clc
            lda         wn
            adc         #2
            sta         p1
            lda         wn + 1
            adc         #0
            sta         p1 + 1
            pla
            jmp         list_alloc_word

; W_VAR: the variable's list ($n: $*'s nth), or its subscripts' words
x_var:
            jsr         var_list                            ; wa: its list
            ldy         #3
            lda         (wn),Y
            tax
            iny
            lda         (wn),Y
            bne         :+
            cpx         #0
            bne         :+
            lda         wa
            ldx         wa + 1
            rts
:
            sta         sub_hi                              ; Subscripts: each a number, or n-m
            stx         sub_lo
            PUSHW       wa
            lda         sub_lo
            ldx         sub_hi
            jsr         expand_one_list                     ; (Their words, plain)
            sta         wb
            stx         wb + 1
            PULLW       wa
            jsr         list_start
            pha
            phx
@sub:
            lda         (wb)
            cmp         #LIST_END
            beq         @end
            jsr         subscript                           ; (Its words appended)
            lda         wb
            ldx         wb + 1
            jsr         list_next
            sta         wb
            stx         wb + 1
            bra         @sub

@end:
            jsr         list_end
            plx
            pla
            rts

; W_COUNT: how many words the variable has, as a word
x_count:
            jsr         var_list
            lda         wa
            ldx         wa + 1
            jsr         list_count
            sta         num
            stz         num + 1
            jmp         num_word

; W_FLAT: the variable's words, one word (spaces between them)
x_flat:
            jsr         var_list
            lda         wa
            ldx         wa + 1
            jmp         list_flat

; W_CAT: the two lists joined
x_cat:
            ldy         #1
            jsr         sub_expand
            PUSHW       wa                                  ; (wa: the left's)
            ldy         #3
            jsr         sub_expand
            MOVR        wb, wa
            PULLW       wa
            jmp         concat

; W_LIST: its words' lists, one after another
x_list:
            ldy         #1
            lda         (wn),Y
            tax
            iny
            lda         (wn),Y
            pha
            txa
            plx
            jmp         expand_raw_list

; W_BQ: the command's output, split
x_bq:
            lda         wn
            ldx         wn + 1
            jsr         run_capture                         ; (exec.s: wa = its output, num its length)
            jmp         split_ifs

; Node wn's word at offset .Y expanded: wa = its list (wn kept)
sub_expand:
            PUSHW       wn
            lda         (wn),Y
            tax
            iny
            lda         (wn),Y
            pha
            txa
            plx
            jsr         expand
            sta         wa
            stx         wa + 1
            PULLW       wn
            rts

; wa = the list of the variable node wn names (a name of digits: $*'s word)
var_list:
            PUSHW       wn
            ldy         #1                                  ; The name: a W_LIT
            lda         (wn),Y
            sta         p0
            iny
            lda         (wn),Y
            sta         p0 + 1
            ldy         #1
            lda         (p0),Y                              ; (Its length)
            sta         sub_lo
            clc
            lda         p0
            adc         #2
            sta         p1
            lda         p0 + 1
            adc         #0
            sta         p1 + 1
            lda         sub_lo
            jsr         digits_of                           ; A number?  $*'s word ($0: a variable of its own)
            bcs         @named
            lda         num
            beq         @named
            LDR         p1, s_star
            lda         #1
            jsr         var_get
            sta         wa
            stx         wa + 1
            PULLW       wn
            lda         num
            jsr         pick
            sta         wa
            stx         wa + 1
            rts

@named:
            lda         sub_lo
            jsr         var_get
            jsr         list_dup                            ; (A copy: the variable may change meanwhile)
            sta         wa
            stx         wa + 1
            PULLW       wn
            rts

; The list .A/.X copied into the arena.  OUT: .A/.X
list_dup:
            sta         we
            stx         we + 1
            jsr         list_start
            pha
            phx
@word:
            lda         (we)
            cmp         #LIST_END
            beq         @end
            pha
            clc
            lda         we
            adc         #1
            sta         p1
            lda         we + 1
            adc         #0
            sta         p1 + 1
            pla
            jsr         list_append_word
            lda         we
            ldx         we + 1
            jsr         list_next
            sta         we
            stx         we + 1
            bra         @word

@end:
            jsr         list_end
            plx
            pla
            rts

; C = 0, num = the number if the .A bytes at p1 are all digits (and some)
digits_of:
            stz         num
            stz         num + 1
            tax
            beq         @no
            ldy         #0
:
            lda         (p1),Y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @no
            pha
            lda         num                                 ; num * 10 + it (8 bits: subscripts are small)
            asl
            asl
            adc         num
            asl
            sta         num
            pla
            clc
            adc         num
            sta         num
            iny
            dex
            bne         :-
            clc
            rts

@no:
            sec
            rts

; A list of the word number .A (1 on) of the list wa: one word, or none.  OUT: .A/.X
pick:
            sta         wl
            jsr         list_start
            pha
            phx
            lda         wl
            beq         @end
            jsr         nth                                 ; (p1, .A: it; C = 1: none)
            bcs         @end
            jsr         list_append_word
@end:
            jsr         list_end
            plx
            pla
            rts

; Word wl (1 on) of list wa: p1 its text, .A its length.  OUT: C = 0; or C = 1: there isn't one
nth:
            MOVR        we, wa
            lda         wl
            sta         wk
            beq         @none
@word:
            lda         (we)
            cmp         #LIST_END
            beq         @none
            dec         wk
            beq         @this
            lda         we
            ldx         we + 1
            jsr         list_next
            sta         we
            stx         we + 1
            bra         @word

@this:
            clc
            lda         we
            adc         #1
            sta         p1
            lda         we + 1
            adc         #0
            sta         p1 + 1
            lda         (we)
            clc
            rts

@none:
            sec
            rts

; Subscript wb (a word: n, or n-m) of list wa: its words appended to the list at the arena's top
subscript:
            lda         (wb)
            sta         mpl                                 ; (Its length)
            ldy         #0                                  ; n, then - and m (or the last: n-)
            jsr         sub_num
            sta         wl
            sta         mi
            cpy         mpl
            beq         @one
            iny
            lda         (wb),Y
            cmp         #'-'
            bne         @bad
            jsr         sub_num
            sta         mj
            cpy         mpl
            bne         @bad
            lda         mj                                  ; (n-: to the end)
            bne         @range
            lda         #$FF
            sta         mj
@range:
            lda         mi
            cmp         mj
            beq         :+
            bcs         @done
:
            lda         mi
            sta         wl
            jsr         nth
            bcs         @done
            jsr         list_append_word
            lda         mi
            cmp         #$FF
            beq         @done
            inc         mi
            bra         @range

@one:
            jsr         nth
            bcs         @done
            jmp         list_append_word

@bad:
            LDR         r0, s_sub
            jmp         rc_error

@done:
            rts

; Subscript wb's number from byte .Y + 1: .A (Y past it)
sub_num:
            stz         num
@digit:
            cpy         mpl
            beq         @done
            iny
            lda         (wb),Y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @back
            pha
            lda         num
            asl
            asl
            adc         num
            asl
            sta         num
            pla
            clc
            adc         num
            sta         num
            bra         @digit

@back:
            dey
@done:
            lda         num
            rts

; num as a word in decimal: a list of one.  OUT: .A/.X
num_word:
            LDR         p1, wbuf
            jsr         rc_number                           ; (rc.s: wbuf = its digits, .A their count)
            jmp         list_alloc_word

; The list .A/.X as one word, spaces between its words.  OUT: .A/.X = a list of one
list_flat:
            sta         we
            stx         we + 1
            ldx         #0
@word:
            lda         (we)
            cmp         #LIST_END
            beq         @end
            tay
            cpx         #0                                  ; (A space before all but the first)
            beq         :+
            lda         #' '
            jsr         wput
:
            tya
            beq         @next
            ldy         #1
:
            lda         (we),Y
            jsr         wput
            tya
            cmp         (we)
            iny
            bcc         :-
@next:
            phx
            lda         we
            ldx         we + 1
            jsr         list_next
            sta         we
            stx         we + 1
            plx
            bra         @word

@end:
            LDR         p1, wbuf
            txa
            jmp         list_alloc_word

; .A into wbuf at .X (WORD_MAX at most: an error past it).  Keeps .Y
wput:
            cpx         #WORD_MAX
            bcs         long
            sta         wbuf,X
            inx
            rts

; A word too long: the command ended (rc_error)
long:
            LDR         r0, s_long
            jmp         rc_error

; The lists wa and wb joined (^).  OUT: .A/.X
concat:
            lda         wa
            ldx         wa + 1
            jsr         list_count
            sta         mi
            lda         wb
            ldx         wb + 1
            jsr         list_count
            sta         mj
            lda         mi
            beq         @null
            lda         mj
            beq         @null
            jsr         list_start
            pha
            phx
            lda         mi
            cmp         mj
            beq         @pairs
            cmp         #1
            beq         @left1
            lda         mj
            cmp         #1
            beq         @right1
            LDR         r0, s_mismatch
            jmp         rc_error

@null:
            LDR         r0, s_null
            jmp         rc_error

@pairs:                                                     ; Each with its pair
            MOVR        we, wb
:
            lda         (wa)
            cmp         #LIST_END
            beq         @end
            jsr         join2
            lda         wa
            ldx         wa + 1
            jsr         list_next
            sta         wa
            stx         wa + 1
            lda         we
            ldx         we + 1
            jsr         list_next
            sta         we
            stx         we + 1
            bra         :-

@left1:                                                     ; wa's one before each of wb's
            MOVR        we, wb
:
            lda         (we)
            cmp         #LIST_END
            beq         @end
            jsr         join2
            lda         we
            ldx         we + 1
            jsr         list_next
            sta         we
            stx         we + 1
            bra         :-

@right1:                                                    ; Each of wa's before wb's one
            MOVR        we, wb
:
            lda         (wa)
            cmp         #LIST_END
            beq         @end
            jsr         join2
            lda         wa
            ldx         wa + 1
            jsr         list_next
            sta         wa
            stx         wa + 1
            bra         :-

@end:
            jsr         list_end
            plx
            pla
            rts

; wa's word then we's, one word appended to the list at the arena's top
join2:
            ldx         #0
            lda         (wa)
            beq         @second
            ldy         #1
:
            lda         (wa),Y
            jsr         wput
            tya
            cmp         (wa)
            iny
            bcc         :-
@second:
            lda         (we)
            beq         @done
            ldy         #1
:
            lda         (we),Y
            jsr         wput
            tya
            cmp         (we)
            iny
            bcc         :-
@done:
            LDR         p1, wbuf
            txa
            jmp         list_append_word

; ****************************************************************************
; Lists of words

; The word list at .A/.X expanded, globbed: one list.  OUT: .A/.X
expand_list:
            sec
            bra         xlist

; The same, not globbed (its markers kept)
expand_raw_list:
            clc
xlist:
            sta         sub_lo                              ; (The list, a moment)
            stx         sub_hi
            php                                             ; (C: glob)
            PUSHW       wc
            PUSHW       wd
            PUSHW       wn
            lda         sub_lo
            sta         wn
            lda         sub_hi
            sta         wn + 1
            stz         wc
            stz         wc + 1
@word:
            lda         wn
            ora         wn + 1
            beq         @done
            PUSHW       wn
            ldy         #2                                  ; Its word, expanded
            lda         (wn),Y
            tax
            iny
            lda         (wn),Y
            pha
            txa
            plx
            jsr         expand
            sta         wa
            stx         wa + 1
            PULLW       wn
            tsx                                             ; (Glob?  The flags, under the 6 pushed)
            lda         $0107,X
            and         #$01
            beq         :+
            PUSHW       wn
            lda         wa
            ldx         wa + 1
            jsr         glob_list
            sta         wa
            stx         wa + 1
            PULLW       wn
:
            jsr         chain_add
            ldy         #0                                  ; The next
            lda         (wn),Y
            tax
            iny
            lda         (wn),Y
            stx         wn
            sta         wn + 1
            bra         @word

@done:
            jsr         chain_join
            sta         wa
            stx         wa + 1
            PULLW       wn
            PULLW       wd
            PULLW       wc
            plp
            lda         wa
            ldx         wa + 1
            rts

; A list wa added to the chain wc ... wd (a cell: +0 the next, +2 the list)
chain_add:
            lda         #4
            ldx         #0
            jsr         arena_alloc
            sta         p0
            stx         p0 + 1
            ldy         #0
            tya
            sta         (p0),Y
            iny
            sta         (p0),Y
            iny
            lda         wa
            sta         (p0),Y
            iny
            lda         wa + 1
            sta         (p0),Y
            lda         wc
            ora         wc + 1
            bne         :+
            MOVR        wc, p0
            bra         :++
:
            ldy         #0
            lda         p0
            sta         (wd),Y
            iny
            lda         p0 + 1
            sta         (wd),Y
:
            MOVR        wd, p0
            rts

; The chain wc's lists, one list.  OUT: .A/.X
chain_join:
            jsr         list_start
            pha
            phx
            stz         p2                                  ; (Nothing yet: room for the end; the words copied
            stz         p2 + 1                              ;   one by one after the cells)
@cell:
            lda         wc
            ora         wc + 1
            beq         @end
            ldy         #2
            lda         (wc),Y
            sta         we
            iny
            lda         (wc),Y
            sta         we + 1
@word:
            lda         (we)
            cmp         #LIST_END
            beq         @next
            pha
            clc
            lda         we
            adc         #1
            sta         p1
            lda         we + 1
            adc         #0
            sta         p1 + 1
            pla
            jsr         list_append_word
            lda         we
            ldx         we + 1
            jsr         list_next
            sta         we
            stx         we + 1
            bra         @word

@next:
            ldy         #0
            lda         (wc),Y
            tax
            iny
            lda         (wc),Y
            stx         wc
            sta         wc + 1
            bra         @cell

@end:
            jsr         list_end
            plx
            pla
            rts

; The word node .A/.X expanded, its markers made characters.  OUT: .A/.X
expand_one:
            jsr         expand
            jmp         glob_strip

; The word list .A/.X expanded, its markers made characters.  OUT: .A/.X
expand_one_list:
            jsr         expand_raw_list
            jmp         glob_strip

; The list .A/.X with its markers made characters again (in place).  OUT: .A/.X (the same)
glob_strip:
            sta         we
            stx         we + 1
            pha
            phx
@word:
            lda         (we)
            cmp         #LIST_END
            beq         @done
            tax
            beq         @next
            ldy         #1
:
            lda         (we),Y
            jsr         unmark
            sta         (we),Y
            iny
            dex
            bne         :-
@next:
            lda         we
            ldx         we + 1
            jsr         list_next
            sta         we
            stx         we + 1
            bra         @word

@done:
            plx
            pla
            rts

; .A: a marker made its character again
unmark:
            cmp         #G_STAR
            bne         :+
            lda         #'*'
            rts
:
            cmp         #G_QUERY
            bne         :+
            lda         #'?'
            rts
:
            cmp         #G_BRACKET
            bne         :+
            lda         #'['
:
            rts

; ****************************************************************************
; Globbing

; The list .A/.X globbed: a word with markers, the names it matches (none: itself, unmarked).  OUT: .A/.X
glob_list:
            sta         wa
            stx         wa + 1
            MOVR        we, wa                              ; Any markers?  (None: the list as it is)
@look:
            lda         (we)
            cmp         #LIST_END
            beq         @plain
            tax
            beq         @nextw
            ldy         #1
:
            lda         (we),Y
            cmp         #G_BRACKET + 1
            bcc         @globbed
            iny
            dex
            bne         :-
@nextw:
            lda         we
            ldx         we + 1
            jsr         list_next
            sta         we
            stx         we + 1
            bra         @look

@plain:
            lda         wa
            ldx         wa + 1
            rts

@globbed:
            PUSHW       wc
            PUSHW       wd
            stz         wc
            stz         wc + 1
@word:
            lda         (wa)
            cmp         #LIST_END
            beq         @join
            PUSHW       wa
            jsr         glob_word                           ; (wa: its list)
            jsr         chain_add
            PULLW       wa
            lda         wa
            ldx         wa + 1
            jsr         list_next
            sta         wa
            stx         wa + 1
            bra         @word

@join:
            jsr         chain_join
            sta         wa
            stx         wa + 1
            PULLW       wd
            PULLW       wc
            lda         wa
            ldx         wa + 1
            rts

; The word at wa globbed: wa = the names it matches, sorted (none: the word itself, unmarked)
glob_word:
            lda         (wa)                                ; Its text, into gpat (to work from)
            sta         gplen
            tax
            beq         @lit
            ldy         #1
:
            lda         (wa),Y
            sta         gpat - 1,Y
            iny
            dex
            bne         :-
            ldx         gplen                               ; Markers in it?
:
            lda         gpat - 1,X
            cmp         #G_BRACKET + 1
            bcc         @glob
            dex
            bne         :-
@lit:
            jsr         list_start                          ; None: as it is
            pha
            phx
            clc
            lda         wa
            adc         #1
            sta         p1
            lda         wa + 1
            adc         #0
            sta         p1 + 1
            lda         (wa)
            jsr         list_append_word
            jsr         list_end
            plx
            pla
            sta         wa
            stx         wa + 1
            rts

@glob:
            jsr         list_start                          ; The names, into a list (gfound of them)
            sta         gstart
            stx         gstart + 1
            stz         gfound
            stz         gpre                                ; (The path so far: none)
            ldx         #0                                  ; From its first part
            jsr         glob_part
            lda         gfound
            bne         :+
            jsr         list_end                            ; None: the word itself, unmarked
            jsr         @lit
            lda         wa
            ldx         wa + 1
            jmp         glob_strip
:
            jsr         list_end
            lda         gstart
            ldx         gstart + 1
            jsr         sort
            sta         wa
            stx         wa + 1
            rts

; The pattern gpat from byte .X, the path so far gpre (its length): each name that matches appended
glob_part:
            stx         gpi                                 ; This part: from .X to a / or the end
            ldy         #0                                  ; (Markers in it?)
            stz         gmark
:
            cpx         gplen
            beq         :+
            lda         gpat,X
            cmp         #'/'
            beq         :+
            cmp         #G_BRACKET + 1
            bcs         @ch
            inc         gmark
@ch:
            inx
            bra         :-
:
            stx         gpe                                 ; (Its end)
            lda         gmark
            bne         @dir
            ldy         gpre                                ; No markers: as it is, onto the path
            ldx         gpi
:
            cpx         gpe
            beq         :+
            lda         gpat,X
            sta         gpath,Y
            iny
            inx
            bra         :-
:
            sty         gpre
            jmp         @onward

@dir:                                                       ; Markers: the directory's names that match
            lda         gpre                                ; (Its path: gpre, or "." at the start)
            pha
            ldy         gpre
            bne         :+
            lda         #'.'
            sta         gpath
            iny
:
            lda         #0
            sta         gpath,Y
            LDR         r0, gpath
            lda         #O_READ
            jsr         OPEN
            tax
            pla
            bcc         :+
            rts                                             ; (Not a directory, or not there: nothing)
:
            phx                                             ; (Each part's fd: kept on the stack)
            lda         gpi
            pha
            lda         gpe
            pha
            lda         gpre
            pha
@record:
            LDR         r0, grec
            LDR         r1, SR_SIZE
            tsx
            lda         $0104,X
            jsr         READ
            bcs         @close
            cmp         #SR_SIZE
            bne         @close
            tsx                                             ; This part again (a deeper one changed them)
            lda         $0103,X
            sta         gpi
            lda         $0102,X
            sta         gpe
            lda         $0101,X
            sta         gpre
            LDR         p0, grec + SR_NAME                  ; Does its name match?
            jsr         str_len
            sta         msl
            LDR         mp_s, grec + SR_NAME
            clc
            lda         #<gpat
            adc         gpi
            sta         mp_p
            lda         #>gpat
            adc         #0
            sta         mp_p + 1
            sec
            lda         gpe
            sbc         gpi
            sta         mpl
            jsr         match_ps
            bcs         @record
            ldy         gpre                                ; Yes: onto the path ...
            beq         :+
            lda         gpath - 1,Y                         ; (A / after what's there, unless it ends in one)
            cmp         #'/'
            beq         :+
            lda         #'/'
            sta         gpath,Y
            iny
:
            ldx         #0
:
            lda         grec + SR_NAME,X
            beq         :+
            sta         gpath,Y
            iny
            inx
            bra         :-
:
            sty         gpre
            ldx         gpe                                 ; ... and on to the next part
            jsr         onward
            jmp         @record

@close:
            pla
            pla
            pla
            pla
            jmp         CLOSE

@onward:
            ldx         gpe
; The pattern from .X on, after the path so far (gpath, gpre bytes): at its end, the path a name found; else its
; next part globbed
onward:
            cpx         gplen                               ; The end: a name found
            bne         @more
            LDR         p1, gpath
            lda         gpre
            jsr         list_append_word
            inc         gfound
            rts

@more:
            lda         gpre                                ; The / after a part
            tay
            lda         #'/'
            sta         gpath,Y
            inc         gpre
            inx
            cpx         gplen                               ; (A / at its end: that's all)
            bne         :+
            LDR         p1, gpath
            lda         gpre
            jsr         list_append_word
            inc         gfound
            rts
:
            jmp         glob_part

; The list .A/.X sorted (its words, by their bytes), in a new list.  OUT: .A/.X
sort:
            sta         we
            stx         we + 1
            jsr         list_count
            sta         gfound
            jsr         list_start
            pha
            phx
            lda         gfound
            beq         @end
@pick:                                                      ; The least of those not taken yet (a taken one's
            lda         we                                  ;   length: its top bit set)
            sta         wb
            lda         we + 1
            sta         wb + 1
            stz         ws + 1                              ; (ws: the least so far; 0: none)
            stz         ws
@scan:
            lda         (wb)
            cmp         #LIST_END
            beq         @take
            bmi         @skip
            lda         ws + 1
            beq         @less
            jsr         wless                               ; wb before ws?
            bcs         @skip
@less:
            MOVR        ws, wb
@skip:
            lda         (wb)                                ; (Past it: its length without the top bit)
            and         #$7F
            sec
            adc         wb
            sta         wb
            bcc         @scan
            inc         wb + 1
            bra         @scan

@take:
            clc
            lda         ws
            adc         #1
            sta         p1
            lda         ws + 1
            adc         #0
            sta         p1 + 1
            lda         (ws)
            pha
            jsr         list_append_word
            pla
            ora         #$80
            sta         (ws)
            dec         gfound
            bne         @pick
@end:
            jsr         list_end
            plx
            pla
            rts

.assert     PATH_MAX < 128, error, "sort: a name's length has its top bit free"

; C = 0 if word wb sorts before word ws
wless:
            ldy         #1
@byte:
            tya
            dec         a
            cmp         (wb)
            beq         @wend
            cmp         (ws)
            beq         @no                                 ; (ws ended first)
            lda         (wb),Y
            cmp         (ws),Y
            bcc         @yes
            bne         @no
            iny
            bra         @byte

@wend:
            cmp         (ws)
            beq         @no                                 ; (The same)
@yes:
            clc
            rts

@no:
            sec
            rts

; ****************************************************************************
; Patterns

; The pattern (.A bytes at p1: markers its * ? [) against the string (.X bytes at p2).  OUT: C = 0: it matches
match:
            sta         mpl
            stx         msl
            MOVR        mp_p, p1
            MOVR        mp_s, p2
; The same, with mp_p, mp_s, mpl and msl set
match_ps:
            stz         mi
            stz         mj
            stz         mstar
@loop:
            ldy         mi
            cpy         mpl
            beq         @pend
            lda         (mp_p),Y
            cmp         #G_STAR
            bne         @one
            iny                                             ; *: from here on, any length
            sty         mi
            sty         mstar
            lda         mj
            sta         mstars
            bra         @loop

@one:
            ldy         mj                                  ; One character
            cpy         msl
            beq         @back
            cmp         #G_QUERY
            beq         @adv
            cmp         #G_BRACKET
            beq         @class
            cmp         (mp_s),Y
            bne         @back
@adv:
            inc         mi
            inc         mj
            bra         @loop

@class:
            jsr         klass                               ; (mi past it; C = 0: the character's in it)
            bcs         @back
            inc         mj
            bra         @loop

@pend:
            lda         mj                                  ; The pattern's end: the string's too?
            cmp         msl
            beq         @yes
@back:
            lda         mstar                               ; After the last *: one more of the string
            beq         @no
            sta         mi
            inc         mstars
            lda         mstars
            sta         mj
            cmp         msl
            beq         @loop
            bcc         @loop
@no:
            sec
            rts

@yes:
            clc
            rts

; [...] at pattern byte mi (its [): is string byte mj in it?  (~ first: not in it; a-b: a range.)  OUT: mi past the
; ]; C = 0: in it
klass:
            ldy         mj
            lda         (mp_s),Y
            sta         kc
            ldy         mi
            iny
            stz         kn
            stz         kf
            cpy         mpl
            beq         @bad
            lda         (mp_p),Y
            cmp         #'~'
            bne         @item
            inc         kn
            iny
@item:
            cpy         mpl
            beq         @bad
            lda         (mp_p),Y
            cmp         #']'
            beq         @end
            sta         klo
            sta         khi
            iny
            cpy         mpl
            beq         @bad
            lda         (mp_p),Y
            cmp         #'-'
            bne         @test
            iny
            cpy         mpl
            beq         @bad
            lda         (mp_p),Y
            sta         khi
            iny
@test:
            lda         kc
            cmp         klo
            bcc         @item
            lda         khi
            cmp         kc
            bcc         @item
            inc         kf
            bra         @item

@end:
            iny
            sty         mi
            lda         kf
            beq         :+
            lda         #1
:
            eor         kn
            beq         @out
            clc
            rts

@out:
            sec
            rts

@bad:
            LDR         r0, s_class
            jmp         rc_error

; ****************************************************************************
; `{cmd}'s output (wa: num bytes) split at spaces, tabs and new lines: a list.  OUT: .A/.X
split_ifs:
            jsr         list_start
            pha
            phx
@word:
            jsr         @skip
            lda         num
            ora         num + 1
            beq         @end
            ldy         #0                                  ; Its length: to a space, or the end
:
            lda         num + 1
            bne         :+
            cpy         num
            beq         @got
:
            lda         (wa),Y
            jsr         ifs
            bcc         @got
            iny
            cpy         #WORD_MAX
            bne         :--
@got:
            sty         wl
            MOVR        p1, wa
            tya
            jsr         list_append_word
            lda         wl
            jsr         @past
            bra         @word

@end:
            jsr         list_end
            plx
            pla
            rts

@skip:                                                      ; Past spaces
            lda         num
            ora         num + 1
            beq         :+
            lda         (wa)
            jsr         ifs
            bcs         :+
            lda         #1
            jsr         @past
            bra         @skip
:
            rts

@past:                                                      ; .A bytes past: wa on, num less
            pha
            clc
            adc         wa
            sta         wa
            bcc         :+
            inc         wa + 1
:
            pla
            sta         wl
            sec
            lda         num
            sbc         wl
            sta         num
            bcs         :+
            dec         num + 1
:
            rts

; C = 0 if .A is a space, a tab or a new line
ifs:
            cmp         #' '
            beq         :+
            cmp         #9
            beq         :+
            cmp         #LF
            beq         :+
            sec
            rts
:
            clc
            rts

.bss
sub_lo:     .res        1                                   ; (A word's pointer, or a length, a moment)
sub_hi:     .res        1
gpat:       .res        WORD_MAX + 1                        ; A pattern being globbed ...
gplen:      .res        1                                   ;   its length
gpath:      .res        WORD_MAX + 2                        ; The path made so far ...
gpre:       .res        1                                   ;   its length
gstart:     .res        2                                   ; The names' list
gfound:     .res        1                                   ;   how many
gpi:        .res        1                                   ; A part's start ...
gpe:        .res        1                                   ;   and end
gmark:      .res        1
gfd:        .res        1
grec:       .res        SR_SIZE

.rodata
exp_vec:    .word       x_lit, x_var, x_count, x_flat, x_cat, x_list, x_bq
s_star:     .byte       "*"
s_long:     .byte       "word too long", 0
s_null:     .byte       "null list in concatenation", 0
s_mismatch: .byte       "mismatched lists in concatenation", 0
s_sub:      .byte       "bad subscript", 0
s_class:    .byte       "bad [ in a pattern", 0
