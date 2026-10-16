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
; there), s_name called, the caller's part copied back (unless it's still there).  So a part can call the main
; code, the overlays and other parts (each call that swaps costs the copies: a part's bytes, about 14 cycles each);
; but nothing outside part n (the main code, an overlay, another part) may call one of its functions or read its
; data but through a stub (build.js checks it, in the compiler's output).
;
; Before including it: zeropage.inc, BANKREG, _ovbank (the overlays' banks), ova and ovx (bytes kept), SWDEPTH; after
; it, the parts, SWPART n, ov (part n's image in the bank _ovbank[ov - 1]: overlay ov's, as a rule), in order from 1
; (nothing else in RODATA between them), and the stubs.  The main code reads the overlays in before it calls one.

            .bss
swapped:   .res        1                   ; The part in the area now (0: none)
swstk:      .res        SWDEPTH             ; The parts the calls found, to put back
swsp:       .res        1

            .code

; Part .Y copied into the area (its image from its bank, in the window meanwhile; the window put back as it was)
swload:
            sty         swapped
            lda         BANKREG
            pha
            tya                             ; (Its record: .Y times 5)
            sta         tmp1
            asl         a
            asl         a
            adc         tmp1
            tax
            lda         swtab - 5,x         ; Its overlay's bank
            tay
            lda         _ovbank,y
            sta         BANKREG
            lda         swtab - 4,x         ; Where its image is, in the window,
            sta         @src + 1
            sta         @src2 + 1
            lda         swtab - 3,x
            sta         @src + 2
            sta         @src2 + 2
            lda         #<SWSTART           ;   to the area's start
            sta         @dst + 1
            sta         @dst2 + 1
            lda         #>SWSTART
            sta         @dst + 2
            sta         @dst2 + 2
            lda         swtab - 2,x         ; Its bytes past its whole pages
            sta         tmp1
            lda         swtab - 1,x         ; Its whole pages
            tax
            ldy         #0
            txa
            beq         @part
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

; Part .Y in the area, the part there kept, if it's called this (a part's function not yet returned: it's to go on
; when this one's done; but the main code's call keeps none: the part last there stays till another's wanted).
; Keeps .A, .X
sw_in:
            sta         ova
            stx         ovx
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
            rts

; The part the last sw_in kept, in the area again (if there was one, and it's gone).  Keeps .A, .X (a function's
; value; and sreg, a long's)
sw_out:
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
            ldy         #n
            jsr         sw_in
            jsr         .ident(.concat("_s_", .string(name)))
            jmp         sw_out
.endmacro

            .import     __SWAP_START__
SWSTART     = __SWAP_START__
            .rodata
swtab:                                      ; (The parts' records follow it: SWPART 1 ...)
            .code
