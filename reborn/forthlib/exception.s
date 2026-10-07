; ****************************************************************************
; exception.s - HyForth's Exception library (/lib/forth/exception.fl): CATCH and THROW (the core's, which its
; interpreter and QUIT use; ABORT and ABORT" are Core's).  An ior (a file word's, a sys- word's) is a THROW code.

.include "forthlib.inc"

            HEADER      "catch", 0
catch_w:                                                    ; CATCH's, for a program: an error it catches has no
            jsr         catch                               ;   place for QUIT to show
            lda         dlo,x
            ora         dhi,x
            beq         :+
            stz         err_noted
            stz         throw_named
:
            rts

            HEADER      "throw", 0
throw_w:                                                    ; ( k*x n -- k*x | i*x n ): 0 nothing; else to CATCH's
            jmp         throw
