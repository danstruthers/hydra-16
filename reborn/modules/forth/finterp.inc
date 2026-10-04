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
; aren't found (their length with F_HIDDEN never matches); names match in either case
find_wl:
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
            clc                                             ; (Its name: w3)
            lda         w
            adc         #3
            sta         w3
            lda         w + 1
            adc         #0
            sta         w3 + 1
            ldy         #0
@char:
            cpy         tmp3
            beq         @found
            lda         (w2),y
            jsr         upper
            sta         cnt
            lda         (w3),y
            jsr         upper
            cmp         cnt
            bne         @next
            iny
            bra         @char
@found:
            clc
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

            HEADER      "FIND", 0
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
; THROW -13.  The stack checked after each
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
            jsr         find_name
            bcs         @number
            inx
            inx
            jsr         hdr_xt
            lda         state
            beq         @execute
            bit         cnt                                 ; (F_IMMEDIATE)
            bmi         @execute
            jsr         comp_hdr
            bra         @check
@execute:
            jsr         exec_w2
@check:
            bit         intr                                ; Ctrl-C?
            bpl         :+
            jmp         intr_throw
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

            HEADER      "EVALUATE", 0
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

; The source onto the stack (it stays the source, for the caller to change).  Too deep: THROW -258
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
            rts

; The source before (a file's that's ended: closed first).  Keeps .X
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
            rts

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

            HEADER      "QUIT", 0
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
            jsr         refill_in
            bcc         :+
            jmp         bye
:
            lda         #<interpret
            ldy         #>interpret
            PUSHAY
            jsr         catch
            lda         dlo,x
            ora         dhi,x
            bne         @error
            inx
            lda         interactive                         ; (The console's: ok)
            beq         @line
            lda         state
            bne         @line
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

; CATCH's frame: the data stack's pointer, the source stack's depth, and the handler before; handler = the 6502's
; stack pointer at it.  THROW goes back to it: the stacks as they were, and the sources nested since let go of
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

            HEADER      "ABORT", 0
abort:
            lda         #<-1
            jmp         throw_a

            HEADERQ     "ABORT", F_IMMEDIATE
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

; QUIT's: error n (on top, dropped) said, after its place: -1 nothing; -2 ABORT"'s message; -13 the name and ?; an
; ior the system's text for it (after the file's name, INCLUDED's); else the table's text, or "error n"
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
            bne         @table
            lda         throw_name
            sta         w
            lda         throw_name + 1
            sta         w + 1
            lda         throw_nlen
            sta         tmp
            stz         tmp + 1
            jsr         type_w
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
            lda         throw_name
            sta         w
            lda         throw_name + 1
            sta         w + 1
            lda         throw_nlen
            sta         tmp
            stz         tmp + 1
            jsr         type_w
            LDR         w, s_colon
            jsr         type_z
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
            .word       -22 & $FFFF, -24 & $FFFF, -28 & $FFFF, -49 & $FFFF, -50 & $FFFF, -258 & $FFFF, -259 & $FFFF, 0
err_text:   .word       e_3, e_4, e_8, e_10, e_14, e_16, e_17, e_22, e_24, e_28, e_49, e_50, e_258, e_259
e_3:        .byte       "stack overflow", 0
e_4:        .byte       "stack underflow", 0
e_8:        .byte       "dictionary full", 0
e_10:       .byte       "division by zero", 0
e_14:       .byte       "compile only", 0
e_16:       .byte       "no name", 0
e_17:       .byte       "number too long", 0
e_22:       .byte       "control structure mismatch", 0
e_24:       .byte       "invalid numeric argument", 0
e_28:       .byte       "interrupt", 0
e_49:       .byte       "search order full", 0
e_50:       .byte       "search order empty", 0
e_258:      .byte       "sources nested too deeply", 0
e_259:      .byte       "not this forth's library", 0

; ---- The rest

            HEADER      "BYE", 0
bye:                                                        ; (At the console: the line ended, for rc's prompt)
            lda         interactive
            beq         :+
            lda         olen
            beq         :+
            jsr         cr
:
            jsr         flush
            stz         r0
            stz         r0 + 1
            lda         #0
            jmp         EXITS

            HEADER      "ENVIRONMENT?", 0
environmentq:                                               ; ( c-addr u -- false ): none known
            inx
            jmp         zero_tos
