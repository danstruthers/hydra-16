; ****************************************************************************
; unpack - the unpack library: a file of banks, each packed (LZ4's block format, a bank on its own), read into RAM
; banks and unpacked there, in place.  A library module (HT_LIBRARY) of the paged ROM, run in its caller's task
; through XCALL: one entry, at $A030.  Dingus Quest Zero's tasks read their overlays with it as they start
; (games/dq0/ovlpack.js makes the files; games/dq0/ovunpack.inc finds the library and calls it): the card's read the
; fewer bytes, about 270 cycles each, for about 40 cycles a byte unpacked (2026-10-31: the game's start, two thirds of
; it the card's reading).  It has no RAM of its own: its pointers are the call registers (r2-r12), its buffer each
; bank itself.
;
; The file: a bank after another, each 4 bytes, L (its bytes, 0-8192) and C (their packed length; 0: stored as they
; are), low bytes first, then its C bytes (or L).  A packed bank's bytes are read to the bank's end ($A000 - C), and
; unpacked from there to its start, the packer having made sure no byte's written over one not yet read (else it's
; stored); the rest of the bank, L on, is filled with $FF (ld65's fill), so the bank is as the linker made it.
;   The packed form, LZ4's block's: a token (its high four bits a run of literals' length, its low four a match's
; length less 4; 15 in either: more bytes follow, each added, till one under 255), the literals, then (unless the
; block's ended) the match's offset back from where it goes (two bytes, low first) and the match, copied a byte at a
; time forward (so a match may overlap what it makes).  The block ends after a token's literals.

.include "hydra.inc"
.include "hyx2.inc"

            HYX2_LIBRARY "unpack", "HEADER", "ROM"
.code
.assert     * = $A030, lderror, "The unpack library's entry isn't right after its header"
            jmp         unpack_banks                        ; $A030: UNPACK_BANKS

WINDOW      = $8000                                         ; The RAM bank window
WEND        = $A000                                         ;   and its end
BANKREG     = $00                                           ; The task's RAM bank register

; The call registers it uses (r0, r1: READ's)
src         = r2                                            ; The packed bytes' next
dst         = r3                                            ; The bank's next byte
from        = r4                                            ; A copy's source
len         = r5                                            ; A copy's length
tok         = r6                                            ; The token (its low byte)
fd          = r7                                            ; (Its low byte: the file's fd; its high: the banks left)
left        = r7 + 1
bank        = r8                                            ; (Its low byte: the bank)
hl          = r9                                            ; The bank's header: L ...
hc          = r10                                           ;   and C
rbuf        = r11                                           ; rd_all's buffer and count left
rcnt        = r12

.code

; UNPACK_BANKS: the file's banks read into banks .Y on (as many as it has, .X at most), each unpacked.  IN: .A =
; the file's fd (at its first bank's header); .X = the banks at most; .Y = the first bank (the rest after it).  OUT:
; C = 0; or C = 1, .A = an error (READ's; E_IO: the file's bytes not as its headers say).  The bank register left
; at the last bank read.  Modifies .A, .X, .Y, r0-r12
unpack_banks:
            sta         fd
            stx         left
            sty         bank
            bra         @bank

@done:
            clc
            rts

@io:
            lda         #E_IO
            sec
            rts

@bank:
            lda         left
            beq         @done
            lda         bank
            sta         BANKREG
            lda         #<(WEND - 4)                        ; Its header, read into the bank's last 4 bytes
            sta         rbuf
            lda         #>(WEND - 4)
            sta         rbuf + 1
            lda         #4
            sta         rcnt
            stz         rcnt + 1
            jsr         rd_all
            bcc         :+
            cpx         #4                                  ; (None of it: the file's end, the banks all read)
            beq         @done
            bra         @io
:
            ldx         #3
:
            lda         WEND - 4,x
            sta         hl,x                                ; (hl, then hc: r9, r10)
            dex
            bpl         :-
            lda         hc
            ora         hc + 1
            beq         @stored
            sec                                             ; Packed: its bytes read to the bank's end ...
            lda         #<WEND
            sbc         hc
            sta         rbuf
            sta         src
            lda         #>WEND
            sbc         hc + 1
            sta         rbuf + 1
            sta         src + 1
            lda         hc
            sta         rcnt
            lda         hc + 1
            sta         rcnt + 1
            jsr         rd_all
            bcs         @io
            stz         dst                                 ; ... unpacked from its start
            lda         #>WINDOW
            sta         dst + 1
            jsr         unlz
            bra         @fill

@stored:
            stz         rbuf                                ; Stored: its L bytes at its start
            lda         #>WINDOW
            sta         rbuf + 1
            lda         hl
            sta         rcnt
            lda         hl + 1
            sta         rcnt + 1
            jsr         rd_all
            bcs         @io
            clc
            lda         hl
            sta         dst
            lda         hl + 1
            adc         #>WINDOW
            sta         dst + 1
@fill:
            lda         dst                                 ; (Its L bytes, and no more?)
            cmp         hl
            bne         @bad
            lda         dst + 1
            sbc         #>WINDOW
            cmp         hl + 1
            beq         :+
@bad:
            jmp         @io
:
            lda         #$FF                                ; The rest of the bank: the linker's fill
            ldy         dst
            stz         dst
:
            ldx         dst + 1
            cpx         #>WEND
            bcs         :+
            sta         (dst),y
            iny
            bne         :-
            inc         dst + 1
            bra         :-
:
            inc         bank
            dec         left
            jmp         @bank

; rcnt bytes of the file read to rbuf, all of them.  OUT: C = 0; or C = 1, .X = 4 if none came (the file's end) and
; more than 0 were asked for, else not 4.  Modifies .A, .X, .Y, r0, r1, rbuf, rcnt
rd_all:
            lda         rcnt
            ora         rcnt + 1
            beq         @ok
            ldx         #4
@more:
            lda         rbuf
            sta         r0
            lda         rbuf + 1
            sta         r0 + 1
            lda         rcnt
            sta         r1
            lda         rcnt + 1
            sta         r1 + 1
            phx
            lda         fd
            jsr         READ
            ply                                             ; (.Y: 4 till something's come)
            bcs         @fail
            sta         r1                                  ; (The count read)
            stx         r1 + 1
            ora         r1 + 1
            beq         @none
            clc
            lda         rbuf
            adc         r1
            sta         rbuf
            lda         rbuf + 1
            adc         r1 + 1
            sta         rbuf + 1
            sec
            lda         rcnt
            sbc         r1
            sta         rcnt
            lda         rcnt + 1
            sbc         r1 + 1
            sta         rcnt + 1
            ldx         #0
            ora         rcnt
            bne         @more
@ok:
            clc
            rts

@none:
            tya
            tax
            sec
            rts

@fail:
            ldx         #0
            rts

; The packed bytes at src unpacked to dst, till src reaches the window's end.  Modifies .A, .X, .Y, src, dst, from,
; len, tok
unlz:
@token:
            lda         (src)
            sta         tok
            inc         src                                 ; (src_next's, in line: a jsr and rts for each of a
            bne         :+                                  ;   token's three bytes, a sixth of the time unpacking)
            inc         src + 1
:
            lsr                                             ; Its literals' length
            lsr
            lsr
            lsr
            sta         len
            stz         len + 1
            cmp         #15
            bne         :+
            jsr         more_len
:
            lda         src                                 ; The literals: from src
            sta         from
            lda         src + 1
            sta         from + 1
            jsr         copy
            lda         from
            sta         src
            lda         from + 1
            sta         src + 1
            cmp         #>WEND                              ; (The end: src at $A000)
            bcs         @end
            lda         (src)                               ; The match: its offset back from dst
            sta         from
            inc         src
            bne         :+
            inc         src + 1
:
            lda         (src)
            sta         from + 1
            inc         src
            bne         :+
            inc         src + 1
:
            sec
            lda         dst
            sbc         from
            sta         from
            lda         dst + 1
            sbc         from + 1
            sta         from + 1
            lda         tok                                 ; Its length
            and         #15
            sta         len
            stz         len + 1
            cmp         #15
            bne         :+
            jsr         more_len
:
            clc
            lda         len
            adc         #4
            sta         len
            bcc         :+
            inc         len + 1
:
            jsr         copy
            bra         @token
@end:
            rts

; src moved on a byte.  Keeps .A, .X, .Y
src_next:
            inc         src
            bne         :+
            inc         src + 1
:
            rts

; A length's more bytes (after a 15), each added to len, till one under 255.  Modifies .A, .X
more_len:
            lda         (src)
            tax
            jsr         src_next
            clc
            txa
            adc         len
            sta         len
            bcc         :+
            inc         len + 1
:
            cpx         #255
            beq         more_len
            rts

; len bytes copied from (from) to (dst), a byte at a time forward (an overlapping match's repeats made as it goes);
; from and dst moved past them.  Modifies .A, .X, .Y
copy:
            ldy         #0
            ldx         len + 1
            beq         @tail
@page:
            lda         (from),y
            sta         (dst),y
            iny
            bne         @page
            inc         from + 1
            inc         dst + 1
            dex
            bne         @page
@tail:
            ldx         len
            beq         @moved
@byte:
            lda         (from),y
            sta         (dst),y
            iny
            dex
            bne         @byte
            tya                                             ; (Both moved on .Y more)
            clc
            adc         from
            sta         from
            bcc         :+
            inc         from + 1
:
            tya
            clc
            adc         dst
            sta         dst
            bcc         @moved
            inc         dst + 1
@moved:
            rts
