; ****************************************************************************
; wozmon - the base's monitor (docs/design/plans/BASE.md, in reborn/docs): Steve Wozniak's Apple 1 monitor, as the
; V1.8C line had it, with its disassembler; a program in task 1 (the base's init), on the kernel's calls.
;
; Its console: #c/cons (fds 0-2: ser's, task F; raw, consctl's rawon, so it reads each key itself), or, without a
; console driver, the kernel's bring-up console (the serial port, polled).  The prompt is the task, its RAM bank and
; >: T1 00> (a shared bank, $F0 up: its U too, T1 F2(3)> ).  A line holds any number of these, as Woz's did:
;   XXXX            examine: the byte at XXXX (or, in L's mode, the instruction there)
;   XXXX.YYYY       a block: XXXX to YYYY, 8 bytes a line (L: an instruction a line); .YYYY alone goes on from the last
;   XXXX: BB BB     store: the bytes from XXXX on (the examine shows XXXX's old byte first); : BB alone goes on
;   XXXXR           run: a JSR to XXXX (R alone: the last shown); its RTS comes back to the prompt
;   L               the examines show instructions (as as writes them: lib/dis.inc), K bytes again
; Hex digits in upper or lower case; Backspace (or Delete) takes back a character, Escape the line.  Ctrl-C (a note:
; ser sends it) stops what's running, a program R started too, and comes back to the prompt; so do the other notes
; (a BRK in a program: NOTE_BRK).
;   What it sees is task 1's view, as any program's: its RAM ($0000-$7FFF: the monitor's own zero page, $22-$7F,
; and its RAM from $0400, a few hundred bytes; $1000 up is free), its RAM bank at $8000 ($00 selects it), the
; paged ROM's bank at $A000 ($01: the monitor's own module, which it runs in: changing $01 pulls the monitor out
; from under itself), the BIOS ROM's page, the I/O ($FF00 up).  A program put in RAM with : and run with R can call
; the kernel (jsr to the jump table: hydra.inc), SPAWN a module, and so on.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "wozmon", main

IN_MAX          = 127           ; A line's characters, at most
OUT_MAX         = 128           ; The output's buffer
DIS_ROOM        = 64            ; An instruction's text, at most
ESC             = $1B
BS              = $08
DEL             = $7F
M_XAM           = 0             ; mode: examine ...
M_BLOCK         = 1             ;   a block ...
M_STORE         = 2             ;   store
NOTE_FRAME      = 3             ; A note handler's stack, above the task's frame: the note (pha) and its return (jsr)
FR_PCL          = 7             ; The frame's PC (U Y W X A P PC: include/layout.inc's FR_PCL, FR_PCH; the kernel's,
FR_PCH          = 8             ;   kernel/notes.s)

.zeropage
st:         .res        2                                   ; The store's address ...
xam:        .res        2                                   ;   the examine's (the next to show) ...
last:       .res        2                                   ;   the last shown (R's) ...
hv:         .res        2                                   ;   and the number just read
mode:       .res        1                                   ; M_XAM, M_BLOCK, M_STORE
dis:        .res        1                                   ; <> 0: the examines show instructions (L)
inn:        .res        1                                   ; The line's length ...
ini:        .res        1                                   ;   and where it's read to
digits:     .res        1                                   ; The number's digits
outn:       .res        1                                   ; The output's bytes, waiting
cons:       .res        1                                   ; <> 0: fds 0-2 are #c/cons
sp0:        .res        1                                   ; The stack as it started (a note brings it back)
cnt:        .res        1                                   ; <> 0: the next byte examined starts a line
len:        .res        1                                   ; The item examined's length
fresh:      .res        1                                   ; <> 0: nothing examined yet on this line
ptr:        .res        2                                   ; (puts's)

.bss
inbuf:      .res        IN_MAX + 1
outbuf:     .res        OUT_MAX
dtext:      .res        DIS_ROOM

.code
main:
            tsx
            stx         sp0
            stz         dis
            stz         outn
            stz         cons
            stz         xam
            stz         xam + 1
            stz         last
            stz         last + 1
            stz         st
            stz         st + 1
            LDR         r0, notes                           ; (A note: back to the prompt)
            jsr         NOTIFY
            LDR         r0, s_cons                          ; Fds 0-2: the console, raw
            lda         #O_RDWR
            jsr         OPEN
            bcs         @polled                             ; (No console driver: the bring-up console's)
            jsr         DUP
            lda         #0
            jsr         DUP
            LDR         r0, s_consctl                       ; (Kept open: raw lasts while it is)
            lda         #O_WRITE
            jsr         OPEN
            bcs         :+
            pha                                             ; (Its fd)
            LDR         r0, s_rawon
            LDR         r1, 5
            pla
            jsr         WRITE
:
            inc         cons
@polled:
            LDR         r0, s_hello
            jsr         puts
            ; (On to the prompt)

; The prompt, and a line read; then its items, done
restart:
            ldx         sp0                                 ; (A note's way back too: its frame's PC)
            txs
            cld
            cli
prompt:
            jsr         crlf
            lda         #'T'
            jsr         putc
            lda         T_REGISTER
            and         #$0F
            jsr         hexdigit
            lda         #' '
            jsr         putc
            lda         RAM_BANK
            jsr         hexbyte
            lda         RAM_BANK                            ; (A shared bank: which, U's)
            cmp         #$F0
            bcc         :+
            lda         #'('
            jsr         putc
            lda         U_REGISTER
            and         #$0F
            jsr         hexdigit
            lda         #')'
            jsr         putc
:
            lda         #'>'
            jsr         putc
            lda         #' '
            jsr         putc
            stz         inn
@key:
            jsr         getc
            bcs         restart                             ; (The end of the input, an error: again)
            cmp         #CR
            beq         @line
            cmp         #LF
            beq         @line
            cmp         #ESC
            beq         @esc
            cmp         #BS
            beq         @back
            cmp         #DEL
            beq         @back
            cmp         #' '                                ; (Other controls: nothing)
            bcc         @key
            ldx         inn
            cpx         #IN_MAX
            bcs         @key
            sta         inbuf,X
            inc         inn
            jsr         putc                                ; (Its echo)
            bra         @key

@back:
            lda         inn
            beq         @key
            dec         inn
            lda         #BS
            jsr         putc
            lda         #' '
            jsr         putc
            lda         #BS
            jsr         putc
            bra         @key

@esc:
            lda         #'\'
            jsr         putc
            jmp         prompt

@line:
            ldx         inn
            lda         #CR                                 ; (Its end)
            sta         inbuf,X
            stz         ini
            lda         #M_XAM
            sta         mode
            lda         #1
            sta         fresh
            jsr         items
            jmp         prompt

; ****************************************************************************
; The line's items, Woz's way
items:
@next:
            ldx         ini
            lda         inbuf,X
            cmp         #CR
            bne         :+
            rts
:
            cmp         #'a'                                ; (Lower case as upper)
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
:
            cmp         #'.'
            bne         :+
            lda         #M_BLOCK
            bra         @mode
:
            cmp         #':'
            bne         :+
            lda         #M_STORE
@mode:
            sta         mode
            inc         ini
            bra         @next
:
            cmp         #'R'
            bne         :+
            inc         ini
            jsr         run
            bra         @next
:
            cmp         #'L'
            bne         :+
            lda         #1
            sta         dis
            inc         ini
            bra         @next
:
            cmp         #'K'
            bne         :+
            stz         dis
            inc         ini
            bra         @next
:
            jsr         number                              ; A number?
            bcc         @number
            inc         ini                                 ; (A space, or anything else: passed over)
            bra         @next

@number:
            lda         mode
            cmp         #M_STORE
            bne         @examine
            lda         hv                                  ; Store: its low byte, and on
            sta         (st)
            inc         st
            bne         @next
            inc         st + 1
            bra         @next

@examine:
            cmp         #M_BLOCK
            beq         @block
            lda         hv                                  ; Examine: from here (and store here), a line of its
            sta         xam                                 ;   own
            sta         st
            lda         hv + 1
            sta         xam + 1
            sta         st + 1
            lda         #1
            sta         cnt
            bra         :+
@block:
            lda         fresh                               ; (A block: from xam on to hv, on the examine's line;
            sta         cnt                                 ;   .YYYY alone, a line of its own)
:
            jsr         examine
            stz         fresh
            lda         #M_XAM
            sta         mode
            jmp         @next

; A hex number at ini: hv (its last 4 digits), ini past it.  OUT: C = 1, none there
number:
            stz         hv
            stz         hv + 1
            stz         digits
@digit:
            ldx         ini
            lda         inbuf,X
            cmp         #'a'
            bcc         :+
            and         #$DF
:
            sec
            sbc         #'0'
            cmp         #10
            bcc         @got
            sbc         #'A' - '0' - 10                     ; (C = 1)
            cmp         #10
            bcc         @end
            cmp         #16
            bcs         @end
@got:
            ldx         #4
:
            asl         hv
            rol         hv + 1
            dex
            bne         :-
            ora         hv
            sta         hv
            inc         digits
            inc         ini
            bra         @digit

@end:
            lda         digits                              ; (None: C = 1)
            beq         :+
            clc
            rts
:
            sec
            rts

; ****************************************************************************
; Examine from xam to hv (inclusive), xam left after the last: bytes, 8 a line (a line's address at each multiple of
; 8, and the first if cnt <> 0), or instructions, a line each
examine:
@item:
            lda         dis
            bne         @insn
            lda         cnt                                 ; A byte: a new line at the first, or a multiple of 8
            bne         @addr
            lda         xam
            and         #7
            bne         @byte
@addr:
            jsr         crlf
            jsr         address
@byte:
            stz         cnt
            lda         #' '
            jsr         putc
            lda         (xam)
            jsr         hexbyte
            lda         #1
            sta         len
            bra         @on

@insn:                                                      ; An instruction: its line
            jsr         crlf
            jsr         address
            lda         xam                                 ; Its text (dis.inc's as_dis)
            sta         r0
            sta         r1
            lda         xam + 1
            sta         r0 + 1
            sta         r1 + 1
            LDR         r2, dtext
            stz         r3
            stz         r3 + 1
            lda         #0
            jsr         as_dis
            sta         len
            ldy         #0                                  ; Its bytes, 3 columns ...
@hex:
            lda         #' '
            jsr         putc
            cpy         len
            bcs         @none
            lda         (xam),Y
            jsr         hexbyte
            bra         :+
@none:
            lda         #' '
            jsr         putc
            lda         #' '
            jsr         putc
:
            iny
            cpy         #3
            bcc         @hex
            lda         #' '                                ;   then the text
            jsr         putc
            jsr         putc
            ldy         #0
:
            lda         dtext,Y
            beq         @on
            jsr         putc
            iny
            bra         :-

@on:                                                        ; On past it; done once past hv (or past $FFFF)
            lda         xam                                 ; (R runs the last shown)
            sta         last
            lda         xam + 1
            sta         last + 1
            clc
            lda         xam
            adc         len
            sta         xam
            lda         xam + 1
            adc         #0
            sta         xam + 1
            bcs         @done                               ; (Past $FFFF)
            lda         hv                                  ; hv >= the next: on
            cmp         xam
            lda         hv + 1
            sbc         xam + 1
            bcc         @done
            jmp         @item
@done:
            rts

; "XXXX:", xam's
address:
            lda         xam + 1
            jsr         hexbyte
            lda         xam
            jsr         hexbyte
            lda         #':'
            jmp         putc

; R: a JSR to the last address shown (XXXXR: XXXX); its RTS comes back
run:
            jsr         flush
            jsr         @go
            jmp         flush
@go:
            jmp         (last)

; ****************************************************************************
; A note (Ctrl-C, a BRK, ...): the task's frame on the stack, under the handler's own; its PC made restart's, so it
; goes on there (the stack, sp0's again)
notes:
            tsx
            lda         #<restart
            sta         $0100 + NOTE_FRAME + FR_PCL,X
            lda         #>restart
            sta         $0100 + NOTE_FRAME + FR_PCH,X
            stz         outn                                ; (What was waiting to go out: gone)
            clc
            rts

; ****************************************************************************
; The console: out through a buffer (a write a line, or as it fills: fd 1's), or the bring-up console's PUTC; in,
; a key at a time (GETC: fd 0's, or the bring-up console's)

getc:
            jsr         flush
            jmp         GETC

crlf:
            lda         #CR
            jsr         putc
            lda         #LF
putc:                                                       ; Keeps .A, .X, .Y
            phx
            ldx         outn
            sta         outbuf,X
            inx
            stx         outn
            cpx         #OUT_MAX
            plx                                             ; (C: full)
            bcc         :+
            jmp         flush
:
            rts

; What's waiting, out.  Keeps .A, .X, .Y
flush:
            pha
            phx
            phy
            lda         outn
            beq         @done
            ldx         cons
            bne         @write
            ldx         #0                                  ; (The bring-up console's: a byte at a time)
:
            lda         outbuf,X
            jsr         PUTC
            inx
            cpx         outn
            bne         :-
            bra         @empty
@write:
            LDR         r0, outbuf
            lda         outn
            sta         r1
            stz         r1 + 1
            lda         #1
            jsr         WRITE
@empty:
            stz         outn
@done:
            ply
            plx
            pla
            rts

; A string at r0, out (r0 copied: flush's write has it)
puts:
            MOVR        ptr, r0
            ldy         #0
:
            lda         (ptr),Y
            beq         :+
            jsr         putc
            iny
            bne         :-
:
            rts

hexbyte:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         hexdigit
            pla
            and         #$0F
hexdigit:
            cmp         #10
            bcc         :+
            adc         #'A' - '0' - 10 - 1                 ; (C = 1)
:
            adc         #'0'
            jmp         putc

; ****************************************************************************
; The disassembler: lib/dis.inc and lib/w65c02.inc, the asm library's (HydraOS's) too
.include "w65c02.inc"
.include "dis.inc"

.rodata
s_cons:     .byte       "#c/cons", 0
s_consctl:  .byte       "#c/consctl", 0
s_rawon:    .byte       "rawon"
s_hello:    .byte       CR, LF, "Hydra-16 monitor (Woz's): XXXX examine, XXXX.YYYY a block, XXXX: BB store, XXXXR run;"
            .byte       CR, LF, "L instructions, K bytes; Escape the line, Ctrl-C back here.", 0
