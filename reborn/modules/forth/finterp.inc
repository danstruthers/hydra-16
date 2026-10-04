; ****************************************************************************
; finterp.inc - forth's text interpreter, QUIT, CATCH and THROW, and the words about them (included by forth.s)

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

; The word named ( c-addr u ) (kept on the stack), in the list: C = 0, w its header; or C = 1.  Hidden ones aren't
; found (their length with F_HIDDEN never matches); names match in either case
find_name:
            lda         latest
            sta         w
            lda         latest + 1
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
            bcc         :+
            dex
            jmp         zero_tos
:
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
            lda         src_addr                            ; (The source before: on the return stack)
            pha
            lda         src_addr + 1
            pha
            lda         src_len
            pha
            lda         src_len + 1
            pha
            lda         to_in
            pha
            lda         to_in + 1
            pha
            lda         src_id
            pha
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
            lda         #$FF
            sta         src_id
            sta         src_id + 1
            jsr         interpret
            pla
            sta         src_id
            sta         src_id + 1
            pla
            sta         to_in + 1
            pla
            sta         to_in
            pla
            sta         src_len + 1
            pla
            sta         src_len
            pla
            sta         src_addr + 1
            pla
            sta         src_addr
            rts

; ---- QUIT, CATCH and THROW

            HEADER      "QUIT", 0
quit:                                                       ; The return stack emptied, and the user's lines
            ldx         rsp0
            txs
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
            ldx         rsp0                                ; Both stacks emptied, interpreting, stdin
            txs
            ldx         #DS_N
            stz         state
            stz         state + 1
            stz         leaves
            stz         leaves + 1
            stz         src_id
            stz         src_id + 1
            bra         @line

s_ok:       .byte       " ok", LF, 0

            HEADER      "CATCH", 0
catch:                                                      ; ( ... xt -- ... 0 | ... n )
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            txa                                             ; (Its frame: the data stack's pointer, the handler
            pha                                             ;   before: the 6502's stack pointer here, handler)
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
            dex
            jmp         zero_tos

exec_w:
            jmp         (w)

            HEADER      "THROW", 0
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
throw_tmp:                                                  ; THROW tmp: the stacks as CATCH had them, n on top
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

            HEADER      "ABORT", 0
abort:
            lda         #<-1
            bra         throw_a

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
            bra         throw_a
:
            rts

; QUIT's: error n (on top, dropped) said: -1 nothing; -2 ABORT"'s message; -13 the name and ?; else its text
show_error:
            lda         dlo,x
            ldy         dhi,x
            inx
            cpy         #$FF
            bne         @other
            cmp         #<-1
            beq         @done
            cmp         #<-2
            bne         :+
            lda         abort_msg
            sta         w
            lda         abort_msg + 1
            sta         w + 1
            lda         (w)
            sta         tmp
            stz         tmp + 1
            inc         w
            bne         @type
            inc         w + 1
            bra         @type
:
            cmp         #<-13
            bne         :+
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
:
            ldy         #0                                  ; One of the table's?
@find:
            lda         err_code,y
            beq         @other_a
            cmp         dlo - 1,x                           ; (Its low byte: the cell just dropped)
            beq         @found
            iny
            bra         @find
@found:
            tya
            asl
            tay
            lda         err_text,y
            sta         w
            lda         err_text + 1,y
            sta         w + 1
@z:
            jsr         type_z
            jmp         cr
@other_a:
@other:
            dex                                             ; ("error n": n again)
            LDR         w, s_error
            jsr         type_z
            jsr         dot
            jmp         cr
@type:
            jsr         type_w
            jmp         cr
@done:
            rts

s_undef:    .byte       " ?", 0
s_error:    .byte       "error ", 0
err_code:   .byte       <-3, <-4, <-8, <-10, <-14, <-16, <-17, <-22, <-24, 0
err_text:   .word       e_3, e_4, e_8, e_10, e_14, e_16, e_17, e_22, e_24
e_3:        .byte       "stack overflow", 0
e_4:        .byte       "stack underflow", 0
e_8:        .byte       "dictionary full", 0
e_10:       .byte       "division by zero", 0
e_14:       .byte       "compile only", 0
e_16:       .byte       "no name", 0
e_17:       .byte       "number too long", 0
e_22:       .byte       "control structure mismatch", 0
e_24:       .byte       "invalid numeric argument", 0

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

            HEADER      "WORDS", 0
words:                                                      ; The list's names, newest first
            lda         latest
            sta         w
            lda         latest + 1
            sta         w + 1
@hdr:
            lda         w
            ora         w + 1
            beq         @done
            ldy         #2
            lda         (w),y
            and         #F_HIDDEN
            bne         @next
            lda         (w),y
            and         #LEN_MASK
            sta         cnt
            ldy         #3
:
            lda         (w),y
            jsr         emit_a
            iny
            dec         cnt
            bne         :-
            jsr         space
@next:
            ldy         #1
            lda         (w),y
            pha
            lda         (w)
            sta         w
            pla
            sta         w + 1
            bra         @hdr
@done:
            jmp         cr

            HEADER      "ENVIRONMENT?", 0
environmentq:                                               ; ( c-addr u -- false ): none known
            inx
            jmp         zero_tos
