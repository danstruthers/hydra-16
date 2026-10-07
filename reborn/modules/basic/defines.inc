; ****************************************************************************
; defines.inc - BASIC's constants (included by basic.s)

SPACE_FOR_GOSUB := $50                      ; The stack kept free below FOR, GOSUB and expressions (CHKMEM): the system's
                                            ;   calls and interrupts use the task's stack too
STACK_TOP       := $FA                      ; The stack's top as BASIC sets it (STKINI); $FB-$FF below its start
WIDTH           := 80                       ; The output's width (Z17), and its last comma field's start (Z18: made from it)

MEM_END         = $7F00                     ; BASIC's memory's end without a bank (BREAK's; task F's RAM ends at $7EFF) ...
MEM_BANK_END    = BANK_WINDOW + BANK_SIZE   ;   and with one, its RAM to $7FFF and the bank at $8000 after it
RAM_BANK        = $00                       ; (hw.inc's: the task's bank register, the bank at $8000-$9FFF)
BANK_WINDOW     = $8000
BANK_SIZE       = $2000
INBUF_MAX       = 240                       ; A line's characters at most (the line buffer's page: INPUTBUFFER at its 8th)

BYTES_FP            := 5
BYTES_PER_ELEMENT   := BYTES_FP
BYTES_PER_VARIABLE  := BYTES_FP+2
MANTISSA_BYTES	    := BYTES_FP-1
BYTES_PER_FRAME     := 2*BYTES_FP+8
FOR_STACK1          := 2*BYTES_FP+5
FOR_STACK2          := BYTES_FP+4

MAX_EXPON = 10

STACK               := $0100
STACK2              := STACK

INPUTBUFFERX        = INPUTBUFFER & $FF00

CR                  = 13
LF                  = 10
TAB                 = 9
