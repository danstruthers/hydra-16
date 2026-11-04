; ****************************************************************************
; swap.inc - Dingus Quest Zero's swapped code (a program's bank.s includes it): code that runs in the main RAM, but
; only one part of it at a time, in an area of its own at the RAM's top (NAME.cfg's SWAP: SW1, SW2 ... all at its
; start, under the C stack's least room), each part kept in an overlay's bank (its image linked there: OVn) and
; copied into the area as it's called.  Unlike an overlay's (code in the window, which can't touch the hub or its
; banks while it runs), a part's code is the main RAM's: it reads and writes the hub as the main code does; so a
; command, an event, a task's start, can be moved out of the main RAM whole, as it is, and called through a stub.
;
; A C file's code and read-only data go in part n with #pragma code-name ("SWnCODE"), rodata-name ("SWnRODATA")
; (push and pop round some of a file's functions; and #pragma local-strings (on), so that a function's strings go
; with it); its data and BSS stay in MAIN.  Its functions called from outside it are named s_name, each with a stub
; here, SWFN name, n: the part that called it (if a part did) kept on a little stack, part n copied in (unless it's
; there), s_name called, the caller's part copied back (unless it's still there), all of it sw_call's, a stub only
; its call and the two numbers (6 bytes; 2026-11-04, the swap study's: a stub's 11 before).  So a part can call the main
; code, the overlays and other parts (each call that swaps costs the copies: a part's bytes, about 14 cycles each);
; but nothing outside part n (the main code, an overlay, another part) may call one of its functions or read its
; data but through a stub (build.js checks it, in the compiler's output).
;
; Before including it: zeropage.inc, BANKREG, _ovbank (the overlays' banks), ova and ovx (bytes kept, in the zero page: a stub's call
; is every call of a part's, some 120 cycles, and these are its most used), SWDEPTH (and
; SWFAST, if the parts are copied four pages a pass: a byte some 10 cycles, not 14, for 90 bytes more code; SWMANY,
; if there are more than 51 parts, past what an index of their records reaches: some 35 bytes more code); after it,
; the parts, SWPART n, ov (part n's image in the bank _ovbank[ov - 1]: overlay ov's, as a rule), in order from 1
; (nothing else in RODATA between them), and the stubs.  The main code reads the overlays in before it calls one.

            .zeropage
swapped:    .res        1                   ; The part in the area now (0: none)
swsp:       .res        1
swip:       .res        2                   ; (Where a stub's numbers are: sw_call's)

            .segment    "ONCE"              ; (The BSS is cleared as a task's loaded; the zero page isn't)
swinit:     stz         swapped
            stz         swsp
            rts
            .constructor swinit

            .bss
swstk:      .res        SWDEPTH             ; The parts the calls found, to put back
.ifdef SWMANY
swrec:      .res        5                   ; (The part's record, swload's: SWMANY's)
.define     SWREC(k)    swrec + k
.else
.define     SWREC(k)    swtab - 5 + k,x
.endif

            .code

; Part .Y copied into the area (its image from its bank, in the window meanwhile; the window put back as it was)
swload:
            sty         swapped
            lda         BANKREG
            pha
.ifdef SWMANY
            sty         tmp1                ; (Its record, into swrec: swtab - 5 + .Y times 5, 16 bits)
            stz         @rec + 2
            tya
            asl         a
            rol         @rec + 2
            asl         a
            rol         @rec + 2
            clc
            adc         tmp1
            bcc         :+
            inc         @rec + 2
            clc
:           adc         #<(swtab - 5)
            sta         @rec + 1
            lda         @rec + 2
            adc         #>(swtab - 5)
            sta         @rec + 2
            ldy         #4
@rec:       lda         $FFFF,y
            sta         swrec,y
            dey
            bpl         @rec
.else
            tya                             ; (Its record: .Y times 5)
            sta         tmp1
            asl         a
            asl         a
            adc         tmp1
            tax
.endif
            lda         SWREC(0)            ; Its overlay's bank
            tay
            lda         _ovbank,y
            sta         BANKREG
            lda         SWREC(1)            ; Where its image is, in the window,
            sta         @src + 1
            sta         @src2 + 1
            lda         SWREC(2)
            sta         @src + 2
            sta         @src2 + 2
            lda         #<SWSTART           ;   to the area's start
            sta         @dst + 1
            sta         @dst2 + 1
            lda         #>SWSTART
            sta         @dst + 2
            sta         @dst2 + 2
            lda         SWREC(3)            ; Its bytes past its whole pages
            sta         tmp1
            lda         SWREC(4)            ; Its whole pages
            tax
            ldy         #0
            txa
.ifdef SWFAST
            bne         @quad
            jmp         @part
@quad:      cpx         #4                  ; (Four pages a pass, while there are four: SWFAST's)
            bcc         @src
            lda         @src + 1
            sta         @s0 + 1
            sta         @s1 + 1
            sta         @s2 + 1
            sta         @s3 + 1
            lda         @dst + 1
            sta         @d0 + 1
            sta         @d1 + 1
            sta         @d2 + 1
            sta         @d3 + 1
            lda         @src + 2
            sta         @s0 + 2
            inc         a
            sta         @s1 + 2
            inc         a
            sta         @s2 + 2
            inc         a
            sta         @s3 + 2
            lda         @dst + 2
            sta         @d0 + 2
            inc         a
            sta         @d1 + 2
            inc         a
            sta         @d2 + 2
            inc         a
            sta         @d3 + 2
@s0:        lda         $FFFF,y
@d0:        sta         $FFFF,y
@s1:        lda         $FFFF,y
@d1:        sta         $FFFF,y
@s2:        lda         $FFFF,y
@d2:        sta         $FFFF,y
@s3:        lda         $FFFF,y
@d3:        sta         $FFFF,y
            iny
            bne         @s0
            lda         @src + 2            ; (Four pages on)
            clc
            adc         #4
            sta         @src + 2
            sta         @src2 + 2
            lda         @dst + 2
            clc
            adc         #4
            sta         @dst + 2
            sta         @dst2 + 2
            txa
            sec
            sbc         #4
            tax
            bne         @quad
            beq         @part
.else
            beq         @part
.endif
@src:       lda         $FFFF,y             ; (A page)
@dst:       sta         $FFFF,y
            iny
            bne         @src
            inc         @src + 2
            inc         @dst + 2
            inc         @src2 + 2
            inc         @dst2 + 2
            dex
            bne         @src
@part:      ldx         tmp1
            beq         @done
@src2:      lda         $FFFF,y             ; (What's left)
@dst2:      sta         $FFFF,y
            iny
            dex
            bne         @src2
@done:      pla
            sta         BANKREG
            rts

; A stub's whole work.  Jsr'd by it: the stub's numbers (the part, the function) read from where it was called, its
; caller's return left on the stack for the function's rts to come back through sw_back.  Part n in the area, the part
; there kept, if it's called this (a part's function not yet returned: it's to go on when this one's done; but the
; main code's call keeps none: the part last there stays till another's wanted); the function called, .A and .X (its
; arguments) as they came; then the part kept in the area again (if there was one, and it's gone), .A, .X (its value;
; and sreg, a long's) as it left them.
sw_call:
            sta         ova
            stx         ovx
            pla                             ; (The stub's jsr: the numbers follow it)
            sta         swip
            pla
            sta         swip + 1
            ldy         #2
            lda         (swip),y
            sta         @go + 1
            iny
            lda         (swip),y
            sta         @go + 2
            ldy         #1
            lda         (swip),y
            tay
            lda         #0
            ldx         swsp
            beq         @keep
            lda         swapped
@keep:      sta         swstk,x
            inc         swsp
            cpy         swapped
            beq         @in
            jsr         swload
@in:        lda         ova
            ldx         ovx
@go:        jsr         $FFFF
sw_back:
            sta         ova
            stx         ovx
            dec         swsp
            ldx         swsp
            ldy         swstk,x
            beq         @out
            cpy         swapped
            beq         @out
            jsr         swload
@out:       lda         ova
            ldx         ovx
            rts

; A part: its record (the bank its image is in, _ovbank[ov - 1]; where it is in the window; its size: the bytes past
; its whole pages, its whole pages), from its link (NAME.cfg's SWn: where its code's linked to run, its size)
.macro      SWPART      n, ov
            .pushseg
            .rodata
            .import     .ident(.concat("__SW", .string(n), "CODE_LOAD__"))
            .import     .ident(.concat("__SW", .string(n), "_START__"))
            .import     .ident(.concat("__SW", .string(n), "_LAST__"))
            .byte       ov - 1
            .word       .ident(.concat("__SW", .string(n), "CODE_LOAD__"))
            .word       .ident(.concat("__SW", .string(n), "_LAST__")) - .ident(.concat("__SW", .string(n), "_START__"))
            .popseg
.endmacro

; A stub: name() calls part n's s_name(), its arguments and its value as they are
.macro      SWFN        name, n
            .export     .ident(.concat("_", .string(name)))
            .import     .ident(.concat("_s_", .string(name)))
.ident(.concat("_", .string(name))):
            jsr         sw_call
            .byte       n
            .word       .ident(.concat("_s_", .string(name)))
.endmacro

            .import     __SWAP_START__
SWSTART     = __SWAP_START__
            .rodata
swtab:                                      ; (The parts' records follow it: SWPART 1 ...)
            .code
