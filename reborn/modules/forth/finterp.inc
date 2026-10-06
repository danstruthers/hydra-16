; ****************************************************************************
; finterp.inc - forth's text interpreter, the source stack, QUIT, CATCH and THROW, and the words about them (included
; by forth.s)

; ---- Finding words

; .A in upper case
upper:
            cmp         #'a'
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
:
            rts

; The word named ( c-addr u ) (kept on the stack), in the search order: C = 0, w its header; or C = 1
find_name:
            ldy         #0                                  ; Each word list in the order, the first first
@wl:
            tya
            lsr
            cmp         order_n
            bcs         @done
            lda         order,y
            sta         w
            lda         order + 1,y
            sta         w + 1
            phy
            jsr         find_wl
            ply
            bcc         @done
            iny
            iny
            bra         @wl
@done:
            rts

; The word named ( c-addr u ) (kept on the stack), in word list w: C = 0, w its header; or C = 1.  Hidden ones
; aren't found (their length with F_HIDDEN never matches); names match in either case.  By the index if there is
; one; else each header, newest first
find_wl:
            lda         w + 1
            pha
            lda         w
            pha
            jsr         idx_find                            ; (.A = 0: C its answer)
            bne         :+
            pla
            pla
            rts
:
            pla
            sta         w
            pla
            sta         w + 1
            ldy         #1                                  ; (Its last header)
            lda         (w),y
            pha
            lda         (w)
            sta         w
            pla
            sta         w + 1
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            lda         dhi,x                               ; (Longer than 255: none)
            bne         @none
            lda         dlo,x                               ; (Its length: tmp3)
            sta         tmp3
@hdr:
            lda         w
            ora         w + 1
            beq         @none
            ldy         #2
            lda         (w),y
            and         #F_HIDDEN | LEN_MASK
            cmp         dlo,x
            bne         @next
            jsr         name_is
            bcs         @next
            rts
@next:
            ldy         #1
            lda         (w),y
            pha
            lda         (w)
            sta         w
            pla
            sta         w + 1
            bra         @hdr
@none:
            sec
            rts

; Is header w's name (its length tmp3's) the one at w2, in either case?  C = 0 yes.  w3 its name
name_is:
            clc
            lda         w
            adc         #3
            sta         w3
            lda         w + 1
            adc         #0
            sta         w3 + 1
            ldy         #0
@char:
            cpy         tmp3
            beq         @yes
            lda         (w2),y                              ; (The same: on; else in upper case)
            cmp         (w3),y
            beq         @next
            jsr         upper
            sta         cnt
            lda         (w3),y
            jsr         upper
            cmp         cnt
            bne         @no
@next:
            iny
            bra         @char
@yes:
            clc
            rts
@no:
            sec
            rts

; ---- The word lists' index
;   In a bank of the task's (idx_bank: its last, taken as forth starts), a record for each word list searched (8
; at most, as the search order has): its last header as it was indexed, and a chain for each of 64 hashes of a
; name (idx_hash), of nodes (a header, the next node) newest first: a search reads one chain, not every header.  A
; word made goes in as it's made (idx_add); whatever else changes a word list leaves its record behind its last
; header (a library loaded, a MARKER), and the next search of it makes its chains again (idx_make), from more
; nodes; -lib and lib, which change FORTH's in the middle, start the index again (idx_drop), as does a bank full of
; nodes.  The bank's "ix" says it's the index (a program that wrote there without BANKS_ALLOC: started again).  No
; bank, or more words than it holds: each header searched, as before

IDX_SIG     = BANK_WINDOW                                   ; The bank: "ix" ...
IDX_N       = BANK_WINDOW + 2                               ;   the records in use ...
IDX_FREE    = BANK_WINDOW + 3                               ;   the next node free ...
IDX_RECS    = BANK_WINDOW + 5                               ;   the records: a word list (2), its last header (2),
IDX_RMAX    = ORDER_MAX                                     ;   its chains' first nodes (2 each, 0: none) ...
IDX_RSIZE   = 4 + 128
IDX_TAILS   = IDX_RECS + IDX_RMAX * IDX_RSIZE               ;   the chains' last nodes, as a record's are made ...
IDX_NODES   = (IDX_TAILS + 128 + 3) & $FFFC                 ;   then the nodes, 4 bytes each (1,018) ...
IDX_END     = RA_BUF                                        ;   to the read-ahead buffer (ffile.inc's ra_line) ...
RA_SIZE     = 512
RA_BUF      = (NS_BSS & $FF00) - RA_SIZE                    ;   and nslib's buffers' page (forth.s: newns's)

; The word named ( c-addr u ) (kept on the stack) in word list w, by the index: .A = 0 and C = 0, w its header, or
; C = 1; or .A <> 0, no index (none, or the name in the bank's window, $8000-$9FFF: EVALUATE of a bank's text).
; Keeps tmp
idx_find:
            lda         idx_bank
            cmp         #$FF
            beq         @no
            lda         dhi + 1,x
            and         #$E0
            cmp         #>BANK_WINDOW
            bne         :+
@no:
            lda         #1
            rts
:
            lda         RAM_BANK                            ; The bank, and the word list's record (made again if it
            sta         idx_savb                            ;   isn't current: tmp its chains; tmp and the bank kept
            lda         tmp                                 ;   meanwhile)
            sta         idx_savt
            lda         tmp + 1
            sta         idx_savt + 1
            lda         idx_bank
            sta         RAM_BANK
            jsr         idx_wl
            bcs         @nobank
            lda         dhi,x                               ; (Longer than a name can be, or none: not a word)
            bne         @none
            lda         dlo,x
            beq         @none
            cmp         #LEN_MASK + 1
            bcs         @none
            sta         tmp3
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            jsr         idx_hash                            ; Its chain: each node, its header's length, then its
            lda         (tmp),y                             ;   name
            sta         idx_node
            iny
            lda         (tmp),y
            sta         idx_node + 1
@node:
            lda         idx_node + 1
            beq         @none
            sta         w3 + 1
            lda         idx_node
            sta         w3
            ldy         #3
            lda         (w3),y
            sta         idx_node + 1
            dey
            lda         (w3),y
            sta         idx_node
            dey
            lda         (w3),y
            sta         w + 1
            lda         (w3)
            sta         w
            ldy         #2
            lda         (w),y
            and         #F_HIDDEN | LEN_MASK
            cmp         tmp3
            bne         @node
            jsr         name_is
            bcs         @node
            jsr         idx_out                             ; (C = 0)
            lda         #0
            rts
@none:
            jsr         idx_out
            lda         #0
            sec
            rts
@nobank:
            jsr         idx_out
            lda         #1
            rts

; (idx_find's end: tmp and the bank as they were.  Keeps C)
idx_out:
            lda         idx_savt
            sta         tmp
            lda         idx_savt + 1
            sta         tmp + 1
            lda         idx_savb
            sta         RAM_BANK
            rts

; tmp = word list w's chains (its record's), current (made again if they aren't), the bank selected: C = 0; or
; C = 1, no index (more words than the bank holds: idx_bank $FF, none from now on)
idx_wl:
            lda         IDX_SIG                             ; (Not the index: started again)
            cmp         #'i'
            bne         :+
            lda         IDX_SIG + 1
            cmp         #'x'
            beq         :++
:
            jsr         idx_reset
:
            jsr         idx_rec                             ; Its record, or a new one (the records all in use:
            bcc         @have                               ;   started again)
            lda         IDX_N
            cmp         #IDX_RMAX
            bcc         :+
            jsr         idx_reset
            LDR         tmp, IDX_RECS
:
            inc         IDX_N
            lda         w
            sta         (tmp)
            ldy         #1
            lda         w + 1
            sta         (tmp),y
            bra         @make
@have:
            ldy         #2                                  ; Current: its last header the word list's
            lda         (w)
            cmp         (tmp),y
            bne         @make
            ldy         #1
            lda         (w),y
            ldy         #3
            cmp         (tmp),y
            beq         @ok
@make:
            jsr         idx_make                            ; (The bank full: started again, this one made first)
            bcc         @ok
            jsr         idx_reset
            inc         IDX_N
            LDR         tmp, IDX_RECS
            lda         w
            sta         (tmp)
            ldy         #1
            lda         w + 1
            sta         (tmp),y
            jsr         idx_make
            bcc         @ok
            lda         #$FF
            sta         idx_bank
            rts
@ok:
            clc                                             ; (Its chains: after the word list and its last header)
            lda         tmp
            adc         #4
            sta         tmp
            bcc         :+
            inc         tmp + 1
:
            clc
            rts

; tmp = the record of word list w (the bank selected): C = 0; or C = 1, none (tmp the next free one's place)
idx_rec:
            ldy         #2                                  ; (One of the two found last: idx_last's)
@last:
            lda         w
            cmp         idx_lastw,y
            bne         :+
            lda         w + 1
            cmp         idx_lastw + 1,y
            bne         :+
            lda         idx_lastr,y
            sta         tmp
            lda         idx_lastr + 1,y
            sta         tmp + 1
            clc
            rts
:
            dey
            dey
            bpl         @last
            LDR         tmp, IDX_RECS
            ldy         IDX_N
@rec:
            sec
            dey
            bmi         @done
            lda         (tmp)
            cmp         w
            bne         :+
            phy
            ldy         #1
            lda         (tmp),y
            ply
            cmp         w + 1
            beq         @found
:
            clc
            lda         tmp
            adc         #<IDX_RSIZE
            sta         tmp
            lda         tmp + 1
            adc         #>IDX_RSIZE
            sta         tmp + 1
            bra         @rec
@found:
            lda         idx_lastw                           ; (Found last: the one before it the other)
            sta         idx_lastw + 2
            lda         idx_lastw + 1
            sta         idx_lastw + 3
            lda         idx_lastr
            sta         idx_lastr + 2
            lda         idx_lastr + 1
            sta         idx_lastr + 3
            lda         w
            sta         idx_lastw
            lda         w + 1
            sta         idx_lastw + 1
            lda         tmp
            sta         idx_lastr
            lda         tmp + 1
            sta         idx_lastr + 1
            clc
@done:
            rts

; The index started again (the bank selected): no records, every node free
idx_reset:
            stz         idx_lastw                           ; (None found last)
            stz         idx_lastw + 1
            stz         idx_lastw + 2
            stz         idx_lastw + 3
            lda         #'i'
            sta         IDX_SIG
            lda         #'x'
            sta         IDX_SIG + 1
            stz         IDX_N
            lda         #<IDX_NODES
            sta         IDX_FREE
            lda         #>IDX_NODES
            sta         IDX_FREE + 1
            rts

; The chains of record tmp (word list w's) made: each header, newest first, at its chain's end; its last header
; noted.  C = 1: the bank full.  Keeps w, tmp
idx_make:
            ldy         #4                                  ; No chains
            lda         #0
:
            sta         (tmp),y
            iny
            cpy         #IDX_RSIZE
            bne         :-
            ldy         #127
:
            sta         IDX_TAILS,y
            dey
            bpl         :-
            lda         w                                   ; (w: each header, then the word list again)
            pha
            lda         w + 1
            pha
            ldy         #1                                  ; Its last header
            lda         (w),y
            pha
            ldy         #3
            sta         (tmp),y
            lda         (w)
            sta         w
            ldy         #2
            sta         (tmp),y
            pla
            sta         w + 1
@hdr:
            clc                                             ; (0: the end)
            lda         w + 1
            beq         @done
            jsr         idx_new                             ; Its node (w3): its header, no next
            bcs         @done
            lda         w
            sta         (w3)
            ldy         #1
            lda         w + 1
            sta         (w3),y
            iny
            lda         #0
            sta         (w3),y
            iny
            sta         (w3),y
            jsr         idx_hdr_hash                        ; After its chain's last, or its first
            lda         IDX_TAILS + 1,y
            beq         @first
            sta         w2 + 1
            lda         IDX_TAILS,y
            sta         w2
            phy
            ldy         #2
            lda         w3
            sta         (w2),y
            iny
            lda         w3 + 1
            sta         (w2),y
            ply
            bra         @tail
@first:
            iny                                             ; (After the word list and its last header)
            iny
            iny
            iny
            lda         w3
            sta         (tmp),y
            iny
            lda         w3 + 1
            sta         (tmp),y
            dey
            dey
            dey
            dey
            dey
@tail:
            lda         w3
            sta         IDX_TAILS,y
            lda         w3 + 1
            sta         IDX_TAILS + 1,y
            ldy         #1                                  ; The next header
            lda         (w),y
            pha
            lda         (w)
            sta         w
            pla
            sta         w + 1
            bra         @hdr
@done:
            pla                                             ; (C kept)
            sta         w + 1
            pla
            sta         w
            rts

; Header lasthdr, just made in the compilation word list, into its record at its chain's start, if it has one and
; it was current (the word list's last header before it the record's); else nothing (the next search makes its
; chains again)
idx_add:
            lda         idx_bank
            cmp         #$FF
            beq         @done
            lda         RAM_BANK
            pha
            lda         idx_bank
            sta         RAM_BANK
            lda         IDX_SIG                             ; (Not the index: the next search starts it again)
            cmp         #'i'
            bne         @out
            lda         IDX_SIG + 1
            cmp         #'x'
            bne         @out
            lda         current                             ; Its word list's record
            sta         w
            lda         current + 1
            sta         w + 1
            jsr         idx_rec
            bcs         @out
            lda         lasthdr
            sta         w
            lda         lasthdr + 1
            sta         w + 1
            lda         (w)                                 ; (Its link the record's last header)
            ldy         #2
            cmp         (tmp),y
            bne         @out
            ldy         #1
            lda         (w),y
            ldy         #3
            cmp         (tmp),y
            bne         @out
            jsr         idx_new                             ; Its node, and the record's last header
            bcs         @out
            lda         w
            sta         (w3)
            ldy         #2
            sta         (tmp),y
            dey
            lda         w + 1
            sta         (w3),y
            ldy         #3
            sta         (tmp),y
            jsr         idx_hdr_hash                        ; Its chain's first, before the one that was
            iny
            iny
            iny
            iny
            lda         (tmp),y
            pha
            lda         w3
            sta         (tmp),y
            iny
            lda         (tmp),y
            pha
            lda         w3 + 1
            sta         (tmp),y
            ldy         #3
            pla
            sta         (w3),y
            dey
            pla
            sta         (w3),y
@out:
            pla
            sta         RAM_BANK
@done:
            rts

; The index started again at the next search (-lib's, lib's)
idx_drop:
            lda         idx_bank
            cmp         #$FF
            beq         @done
            lda         RAM_BANK
            pha
            lda         idx_bank
            sta         RAM_BANK
            stz         IDX_SIG
            pla
            sta         RAM_BANK
@done:
            rts

; w3 = a new node, C = 0; or C = 1, the bank full
idx_new:
            lda         IDX_FREE + 1
            cmp         #>IDX_END
            bcs         @done
            sta         w3 + 1
            lda         IDX_FREE
            sta         w3
            clc
            adc         #4
            sta         IDX_FREE
            bcc         @done
            inc         IDX_FREE + 1
            clc
@done:
            rts

; idx_hash of header w's name (w2, tmp3)
idx_hdr_hash:
            ldy         #2
            lda         (w),y
            and         #LEN_MASK
            sta         tmp3
            clc
            lda         w
            adc         #3
            sta         w2
            lda         w + 1
            adc         #0
            sta         w2 + 1
; The chain of the name at w2, tmp3 long: .Y its place (2 bytes each), from its hash: its length * 8, and its first
; and last characters with bit 5 off (a letter in upper case), the sum's low 6 bits.  Keeps w3
idx_hash:
            lda         (w2)                                ; (Bit 5 off: a letter's case, and a few other
            and         #$DF                                ;   characters', the same: the names are compared after)
            sta         cnt
            ldy         tmp3
            dey
            lda         (w2),y
            and         #$DF
            clc
            adc         cnt
            sta         cnt
            lda         tmp3
            asl
            asl
            asl
            clc
            adc         cnt
            and         #63
            asl
            tay
            rts

            HEADER      "find", 0
find:                                                       ; ( c-addr -- c-addr 0 | xt 1 | xt -1 )
            jsr         dup
            jsr         count
            jsr         find_name
            inx
            inx
            bcc         found_xt
            dex
            jmp         zero_tos

; ( x -- xt 1 | xt -1 ): the word whose header is at w, found
found_xt:
            jsr         hdr_xt
            lda         w2
            sta         dlo,x
            lda         w2 + 1
            sta         dhi,x
            dex
            bit         cnt
            bmi         :+
            jmp         true_tos
:
            lda         #1
            sta         dlo,x
            stz         dhi,x
            rts

; ---- The text interpreter

; The source, word by word, from >IN: each executed, or compiled (STATE); a number pushed or compiled; else
; THROW -13; a compile-only word (F_COMPILE) not in a definition, THROW -14.  The stack checked after each
interpret:
@next:
            jsr         parse_name
            lda         dlo,x
            ora         dhi,x
            bne         @word
            inx
            inx
            rts
@word:
            lda         dlo + 1,x                           ; (Its name, for an error)
            sta         throw_name
            lda         dhi + 1,x
            sta         throw_name + 1
            lda         dlo,x
            sta         throw_nlen
            lda         state                               ; (Compiling: a local's name first, a word's or a
            beq         :+                                  ;   number's after: locals.fl's)
            lda         #0
            jsr         loc_call
            bcc         @check
:
            jsr         find_name
            bcs         @number
            inx
            inx
            jsr         hdr_xt
            lda         state
            beq         @interp
            bit         cnt                                 ; (F_IMMEDIATE)
            bmi         @execute
            jsr         comp_hdr
            bra         @check
@interp:
            lda         cnt                                 ; Compile-only?  (F_INLINE, and F_COMPILE in its byte:
            and         #F_INLINE                           ;   hdr_xt's .Y, the xt's offset, past it)
            beq         @execute
            dey
            lda         (w),y
            bpl         @execute
            lda         #<-14
            jmp         throw_a
@execute:
            jsr         exec_w2
@check:
            bit         intr                                ; Ctrl-C, or a note for a program's handler?
            bpl         :+
            jsr         intr_check
:
            cpx         #DS_N + 1                           ; Underflow, overflow
            bcc         :+
            lda         #<-4
            jmp         throw_a
:
            cpx         #4
            bcs         @next
            lda         #<-3
            jmp         throw_a

@number:
            jsr         number                              ; ( n 1 | d 2 | 0 )
            lda         dlo,x
            beq         @undef
            inx
            ldy         state
            beq         @check
            cmp         #1
            beq         :+
            jsr         swap                                ; (A double: its low cell first)
            jsr         literal
:
            jsr         literal
            bra         @check
@undef:
            inx
            jmp         throw_13

; jsr to w2 (an xt)
exec_w2:
            jmp         (w2)

            HEADER      "evaluate", 0
evaluate:                                                   ; ( c-addr u -- ): it as the source, then the one before
            jsr         src_push
            lda         dlo,x
            sta         src_len
            lda         dhi,x
            sta         src_len + 1
            lda         dlo + 1,x
            sta         src_addr
            lda         dhi + 1,x
            sta         src_addr + 1
            inx
            inx
            stz         to_in
            stz         to_in + 1
            stz         src_close
            lda         #$FF
            sta         src_id
            sta         src_id + 1
            jsr         interpret
            jmp         src_pop

; ---- The source stack: the sources a nested one (EVALUATE's, INCLUDE-FILE's) was in, each a copy of the source's
; variables (SRC_SIZE bytes from src_addr)

; The source onto the stack (it stays the source, for the caller to change; not a block's).  Too deep: THROW -258
src_push:
            lda         ssp
            cmp         #SRC_MAX
            bcc         :+
            jmp         throw_deep
:
            jsr         src_rec
            ldy         #SRC_SIZE - 1
:
            lda         src_addr,y
            sta         (w3),y
            dey
            bpl         :-
            inc         ssp
            stz         src_blk
            stz         src_blk + 1
            rts

; The source before (a file's that's ended: closed first; a block's: its buffer again, as one LOADed since may have
; taken it).  Keeps .X
src_pop:
            lda         src_close
            beq         :+
            stx         xsave
            lda         src_id
            jsr         CLOSE
            ldx         xsave
:
            dec         ssp
            lda         ssp
            jsr         src_rec
            ldy         #SRC_SIZE - 1
:
            lda         (w3),y
            sta         src_addr,y
            dey
            bpl         :-
            lda         src_blk
            ora         src_blk + 1
            beq         :+
            lda         #0
            jmp         blk_call
:
            rts

; The Block library's routine (blk_vec: block.fl's), .A the call: 0, src_addr the buffer of block src_blk (read in
; if it isn't in one); 1, the program's ending: the blocks changed written to their file.  Keeps .X.  None: C = 1
blk_call:
            sec
            ldy         blk_vec + 1
            beq         :+
            jmp         (blk_vec)
:
            rts

; As the program ends (BYE, a script's end): the Block library's changed blocks written (its call 1), an error in it
; caught and dropped
blk_end:
            lda         #<blk_end1
            ldy         #>blk_end1
            PUSHAY
            jsr         catch
            inx
            rts

blk_end1:
            lda         #1
            jmp         blk_call

; The sources nested since the stack had .A records let go of (THROW's, QUIT's)
src_unwind:
            pha
@pop:
            pla
            pha
            cmp         ssp
            bcs         @done                               ; (ssp <= it)
            jsr         src_pop
            bra         @pop
@done:
            pla
            rts

; w3 = the source stack's record .A (0 ...).  Keeps .X
src_rec:
            tay
            lda         #<sstack
            sta         w3
            lda         #>sstack
            sta         w3 + 1
@next:
            dey
            bmi         @done
            clc
            lda         w3
            adc         #SRC_SIZE
            sta         w3
            bcc         @next
            inc         w3 + 1
            bra         @next
@done:
            rts

; ---- QUIT, CATCH and THROW

            HEADER      "quit", 0
quit:                                                       ; The return stack emptied, the sources nested let go of,
            ldx         rsp0                                ;   and the user's lines
            txs
            stz         handler
            stz         handler + 1
            lda         #0
            jsr         src_unwind
            ldx         #DS_N
            stz         state
            stz         state + 1
@line:
            lda         interactive                         ; (The console's: the shell's prompt, if it's loaded)
            beq         :+
            jsr         shell_prompt
:
            jsr         refill_in
            bcc         :+
            jmp         bye
:
            lda         #<quit_line
            ldy         #>quit_line
            PUSHAY
            jsr         catch
            lda         dlo,x
            ora         dhi,x
            bne         @error
            inx
            lda         interactive                         ; (The console's: ok, but in the shell, if the line
            beq         @line                               ;   has left it loaded: its prompt instead)
            lda         state
            bne         @line
            lda         #<s_shprompt
            ldy         #>s_shprompt
            jsr         find_hook
            bcc         @line
            LDR         w, s_ok
            jsr         type_z
            bra         @line
@error:
            jsr         show_error
            ldx         rsp0                                ; Both stacks emptied, interpreting
            txs
            ldx         #DS_N
            stz         state
            stz         state + 1
            stz         leaves
            stz         leaves + 1
            bra         @line

s_ok:       .byte       " ok", LF, 0

; A line typed at the console, interpreting: the shell's ((shell-line) ( -- flag ), if it's loaded: true, it took the
; line, an rc command line); else interpreted
quit_line:
            lda         interactive
            beq         @forth
            lda         state
            ora         state + 1
            bne         @forth
            lda         #<s_shline
            ldy         #>s_shline
            jsr         find_hook
            bcs         @forth
            jsr         exec_w2
            lda         dlo,x
            ora         dhi,x
            php
            inx
            plp
            beq         @forth
            rts
@forth:
            jmp         interpret

; The shell's prompt ((shell-prompt) ( -- ), if it's loaded)
shell_prompt:
            lda         #<s_shprompt
            ldy         #>s_shprompt
            jsr         find_hook
            bcs         :+
            jmp         exec_w2
:
            rts

; The word named by the counted string at .A/.Y, in the search order (the shell's hooks: a library's, found by
; name, so they're there only while it's loaded and searched).  OUT: C = 0, w2 its xt; or C = 1
find_hook:
            sta         w
            sty         w + 1
            clc
            adc         #1
            bcc         :+
            iny
:
            PUSHAY
            lda         (w)
            ldy         #0
            PUSHAY
            jsr         find_name
            inx
            inx
            bcs         :+
            jsr         hdr_xt
            clc
:
            rts

s_shline:   .byte       12, "(shell-line)"
s_shprompt: .byte       14, "(shell-prompt)"

; CATCH's frame: the data stack's pointer, the source stack's depth, the locals' frame (lp), and the handler before;
; handler = the 6502's stack pointer at it.  THROW goes back to it: the stacks (and lp) as they were, and the sources
; nested since let go of
catch:                                                      ; ( ... xt -- ... 0 | ... n )
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            txa
            pha
            lda         ssp
            pha
            lda         lp                                  ; (The locals' frame: THROW gives it back)
            pha
            lda         handler + 1
            pha
            lda         handler
            pha
            stx         xsave
            tsx
            stx         handler
            lda         #1
            sta         handler + 1
            ldx         xsave
            jsr         exec_w
            pla                                             ; (Done: the handler before, and 0)
            sta         handler
            pla
            sta         handler + 1
            pla
            pla
            pla
            dex
            jmp         zero_tos

exec_w:
            jmp         (w)

; THROW's (the Exception library's header: exception.fl)
throw:                                                      ; ( k*x n -- k*x | i*x n ): 0 nothing; else to CATCH's
            lda         dlo,x
            ora         dhi,x
            bne         :+
            inx
            rts
:
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
throw_tmp:                                                  ; THROW tmp: its place noted; the stacks and sources as
            jsr         note_where                          ;   CATCH had them, n on top
            lda         handler + 1
            bne         :+
            jmp         quit                                ; (No CATCH: QUIT's start)
:
            ldx         handler
            txs
            pla
            sta         handler
            pla
            sta         handler + 1
            pla
            sta         lp
            pla
            jsr         src_unwind
            pla
            tax
            dex
            lda         tmp
            sta         dlo,x
            lda         tmp + 1
            sta         dhi,x
            rts

; THROW .A (a code from -1 to -255)
throw_a:
            sta         tmp
            lda         #$FF
            sta         tmp + 1
            bra         throw_tmp

; THROW the ior of the system's error .A (-512 less it)
throw_os:
            eor         #$FF
            inc
            sta         tmp
            lda         #$FD
            sta         tmp + 1
            bra         throw_tmp

; THROW -258: sources nested too deeply
throw_deep:
            lda         #<-258
            sta         tmp
            lda         #>-258
            sta         tmp + 1
            bra         throw_tmp

; THROW -13 for the name on the stack ( c-addr u ), or the one already noted (throw_13)
throw_undef:
            lda         dlo + 1,x
            sta         throw_name
            lda         dhi + 1,x
            sta         throw_name + 1
            lda         dlo,x
            sta         throw_nlen
throw_13:
            lda         #<-13
            bra         throw_a

; THROW's: the place of the error noted for QUIT, if it isn't yet: the line of the innermost source that isn't a
; string (a file's, or stdin's when it isn't the console)
note_where:
            lda         err_noted
            bne         @done
            LDR         w3, src_addr                        ; The source, then the ones it's in
            lda         ssp
            sta         cnt
@look:
            ldy         #src_id + 1 - src_addr
            lda         (w3),y
            beq         @found
            dec         cnt
            bmi         @done
            lda         cnt
            jsr         src_rec
            bra         @look
@found:
            dey
            lda         (w3),y
            bne         :+
            lda         interactive
            bne         @done
:
            ldy         #src_line - src_addr
            lda         (w3),y
            sta         err_line
            iny
            lda         (w3),y
            sta         err_line + 1
            ldy         #src_fdep - src_addr
            lda         (w3),y
            sta         err_fdep
            inc         err_noted
@done:
            rts

            HEADER      "abort", 0
abort:
            lda         #<-1
            jmp         throw_a

            HEADERQC    "abort", F_IMMEDIATE
abortquote:
            lda         #'"'
            sta         cnt
            jsr         parse_to
            lda         #<xabortq
            ldy         #>xabortq
            jmp         comp_str

; ABORT"'s: ( flag -- ): true, the message noted and THROW -2
xabortq:
            jsr         inline_str
            lda         dlo,x
            ora         dhi,x
            inx
            cmp         #0
            beq         :+
            lda         w
            sta         abort_msg
            lda         w + 1
            sta         abort_msg + 1
            lda         #<-2
            jmp         throw_a
:
            rts

; QUIT's: error n (on top, dropped) said, after its place: -1 nothing; -2 ABORT"'s message; -13 the name and ?; -14
; the name and the text; an ior the system's text for it (after the file's name, INCLUDED's); else the table's text,
; or "error n"
show_error:
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
            inx
            and         tmp
            cmp         #$FF
            bne         :+
            stz         err_noted
            rts
:
            jsr         show_where
            lda         tmp + 1
            cmp         #$FF
            bne         @os
            lda         tmp
            cmp         #<-2
            bne         :+
            lda         abort_msg
            sta         w
            lda         abort_msg + 1
            sta         w + 1
            lda         (w)
            sta         tmp
            stz         tmp + 1
            jsr         w_inc
            jmp         @type
:
            cmp         #<-13
            beq         :+
            cmp         #<-14
            bne         @table
            jsr         @namec                              ; ("name: compile only")
            LDR         w, e_14
            bra         @z
:
            jsr         @name
            LDR         w, s_undef
            bra         @z
@os:
            cmp         #$FD
            bne         @table
            lda         tmp                                 ; (The system's code: kept)
            eor         #$FF
            inc
            pha
            lda         throw_named                         ; INCLUDED's file's name first
            beq         :+
            stz         throw_named
            jsr         @namec
:
            LDR         r0, wbuf
            pla
            stx         xsave
            jsr         ERRSTR
            ldx         xsave
            LDR         w, wbuf
            bra         @z
@table:
            ldy         #0                                  ; One of the table's?
@find:
            lda         err_code,y
            ora         err_code + 1,y
            beq         @other
            lda         err_code,y
            cmp         tmp
            bne         :+
            lda         err_code + 1,y
            cmp         tmp + 1
            beq         @found
:
            iny
            iny
            bra         @find
@found:
            lda         err_text,y
            sta         w
            lda         err_text + 1,y
            sta         w + 1
@z:
            jsr         type_z
            jmp         cr
@other:
            dex                                             ; ("error n": n again)
            lda         tmp
            sta         dlo,x
            lda         tmp + 1
            sta         dhi,x
            LDR         w, s_error
            jsr         type_z
            jsr         dot
            jmp         cr
@type:
            jsr         type_w
            jmp         cr
@namec:                                                     ; (The name noted, and ": ")
            jsr         @name
            LDR         w, s_colon
            jmp         type_z
@name:                                                      ; (The name noted)
            lda         throw_name
            sta         w
            lda         throw_name + 1
            sta         w + 1
            lda         throw_nlen
            sta         tmp
            stz         tmp + 1
            jmp         type_w

; The error's place, if THROW noted one: "name:line: " (a file INCLUDED), or "line N: ".  Keeps tmp
show_where:
            lda         err_noted
            beq         @done
            stz         err_noted
            lda         tmp + 1
            pha
            lda         tmp
            pha
            lda         err_fdep
            beq         @line
            jsr         lname_w
            lda         (w)
            beq         @line
            sta         tmp
            stz         tmp + 1
            jsr         w_inc
            jsr         type_w
            lda         #':'
            jsr         emit_a
            bra         @num
@line:
            LDR         w, s_line
            jsr         type_z
@num:
            lda         base                                ; (In decimal)
            pha
            lda         #10
            sta         base
            dex
            lda         err_line
            sta         dlo,x
            lda         err_line + 1
            sta         dhi,x
            jsr         u_text
            jsr         type
            pla
            sta         base
            LDR         w, s_colon
            jsr         type_z
            pla
            sta         tmp
            pla
            sta         tmp + 1
@done:
            rts

; w = the name of the file being included at depth .A (1 ...), counted
lname_w:
            dec
            asl
            asl
            asl
            asl
            asl
            clc
            adc         #<lnames
            sta         w
            lda         #>lnames
            adc         #0
            sta         w + 1
            rts

; w on a byte
w_inc:
            inc         w
            bne         :+
            inc         w + 1
:
            rts

s_undef:    .byte       " ?", 0
s_error:    .byte       "error ", 0
s_line:     .byte       "line ", 0
s_colon:    .byte       ": ", 0
err_code:   .word       -3 & $FFFF, -4 & $FFFF, -8 & $FFFF, -10 & $FFFF, -14 & $FFFF, -16 & $FFFF, -17 & $FFFF
            .word       -21 & $FFFF, -22 & $FFFF, -24 & $FFFF, -28 & $FFFF, -49 & $FFFF, -50 & $FFFF, -59 & $FFFF
            .word       -60 & $FFFF, -61 & $FFFF, -258 & $FFFF, -259 & $FFFF, 0
err_text:   .word       e_3, e_4, e_8, e_10, e_14, e_16, e_17, e_21, e_22, e_24, e_28, e_49, e_50, e_59, e_60, e_61
            .word       e_258, e_259
e_3:        .byte       "stack overflow", 0
e_4:        .byte       "stack underflow", 0
e_8:        .byte       "dictionary full", 0
e_10:       .byte       "division by zero", 0
e_14:       .byte       "compile only", 0
e_16:       .byte       "no name", 0
e_17:       .byte       "number too long", 0
e_21:       .byte       "unsupported operation", 0
e_22:       .byte       "control structure mismatch", 0
e_24:       .byte       "invalid numeric argument", 0
e_28:       .byte       "interrupt", 0
e_49:       .byte       "search order full", 0
e_50:       .byte       "search order empty", 0
e_59:       .byte       "allocate failed", 0
e_60:       .byte       "free failed", 0
e_61:       .byte       "resize failed", 0
e_258:      .byte       "sources nested too deeply", 0
e_259:      .byte       "not this forth's library", 0

; ---- The rest

            HEADER      "bye", 0
bye:                                                        ; (At the console: the line ended, for rc's prompt)
            lda         interactive
            beq         :+
            lda         olen
            beq         :+
            jsr         cr
:
            jsr         blk_end
            jsr         flush
            stz         r0
            stz         r0 + 1
            lda         #0
            jmp         EXITS

            HEADER      "environment?", 0
environmentq:                                               ; ( c-addr u -- false ): none known but the locals
            lda         #4                                  ;   library's #LOCALS ( -- n true )
            jsr         loc_call
            bcc         :+
            inx
            jmp         zero_tos
:
            rts
