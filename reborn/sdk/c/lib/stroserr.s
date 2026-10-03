; ****************************************************************************
; stroserr.s - const char* __fastcall__ __stroserror (unsigned char errcode): an error code's text, as the kernel's
; ERRSTR gives it ("not found"), for _poserror and the like.  (It takes the place of cc65's, which looks the code
; up in a table of the target's: here the kernel has the table.)  The text is good till the next call.

            .export     ___stroserror

            .include    "hydra.inc"

            .code

___stroserror:
            pha
            lda         #<text
            sta         r0
            lda         #>text
            sta         r0 + 1
            pla
            jsr         ERRSTR
            lda         #<text
            ldx         #>text
            rts

            .bss
text:       .res        32
