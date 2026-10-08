; ****************************************************************************
; basic.inc - BASIC's values, memory and codes (docs/basic.md, "The design").
;
; Memory.  The task's RAM holds the interpreter's state, the globals (5 bytes each) and the value stack (frames,
; GOSUB's returns and the expressions' values, 5 bytes each); everything else is in the task's RAM banks, each seen
; at $8000-$9FFF in turn, by a table of logical banks (ltab: a logical bank's physical one, 0 none yet) in regions:
; the program's code (its compiled form), the program's text (the buffer at the prompt), the symbols (the names, the
; labels, the line table) and the heap (strings, numbers past 32 bits, arrays, records).  A far address is a logical
; bank and an address in the window ($8000-$9FFF): 3 bytes, the address first; in the code, text and symbol
; regions, a 16-bit region address (its bank in the region, then 13 bits) says the same.

LB_CODE         = 0             ; The code: the program's in logical banks 0-6, the line typed at the prompt's in 7
LB_IMM          = 7
LB_TEXT         = 8             ; The program's text: 8-15
LB_SYM          = 16            ; The symbols: 16-19 (the names, labels' fixups)
LB_LT           = 20            ; The line table: 20-21 (each statement's code address and line)
LB_DATA         = 22            ; DATA's items: 22-23
LB_NSB          = 24            ; The numbers' scratch: their operands and results (num.inc)
LB_TXB          = 25            ; A number's text (DISPLAY's)
LB_HEAP         = 26            ; The heap: 26 on
LB_HEAP_END     = 64
BASE_ROOM       = 96            ; A base string at most, its 0 too (the numbers library's)
LB_MAX          = 64
WIN             = $80           ; (The window's page: $8000)
WIN_END         = $A0
RAM_BANK        = $00           ; (The bank at $8000-$9FFF)

; A value: a tag, then 4 bytes
VT_INT          = 0             ; An integer of 32 bits, signed (little-endian)
VT_NUM          = 1             ; A number in the stored format (spec/numbers.def), at a far address: its length (2
                                ;   bytes), then its bytes
VT_STR          = 2             ; A string at a far address: its length (2 bytes), then its bytes; bank $FF: ""
VT_REF          = 3             ; A slot of the RAM (2 bytes): a parameter given by reference
VT_ARR          = 4             ; An array: its block's far address; bank $FF: not made yet (DIM)
VT_REC          = 5             ; A record: its block's far address; bank $FF: not made yet
VT_LOC          = 6             ; An element (or a field) given by reference: the RAM slot of its array (or record)
                                ;   variable (2 bytes), and the value's index in its block (2)
VT_FRM          = 7             ; A procedure's frame: its return (a code address), the caller's fp
VT_GOS          = 8             ; GOSUB's return: a code address
VT_ERR          = 9             ; An error handler's mark: the error's sp (the error's state is err_*)
VT_FIX          = $B0           ; A fixed decimal of 0-15 places whose digits fit in 32 bits (VT_FIX + places, as
                                ;   the stored format's tag): its digits, signed (little-endian)
VAL_SIZE        = 5
NOBANK          = $FF

; A heap block: its kind (bit 7 the collector's mark), its size (all of it, 2 bytes), the collector's forwarding
; address (3), then its contents, which a value's far address points at (the block's start + HDR)
KB_STR          = 1             ; A string: its length (2), its bytes
KB_NUM          = 2             ; A number: its length (2), its bytes in the stored format
KB_ARR          = 3             ; An array: its dimensions (1), each element's values (1: a record's fields, else 1),
                                ;   each dimension's lowest index (2, signed) and count (2), then its values
KB_REC          = 4             ; A record: its fields' values
HDR             = 6
KB_MARK         = $80

; The compiled code's ops are run.inc's; a code address is 16 bits: its logical bank (the top 3 bits) and 13 bits

; Errors: QuickBASIC's codes (ERR), the system's past 255 (256 + its E_ code, but those QuickBASIC has a code for)
ER_NEXT         = 1             ; NEXT without FOR
ER_SYNTAX       = 2
ER_RETURN       = 3             ; RETURN without GOSUB
ER_DATA         = 4             ; Out of DATA
ER_FUNC         = 5             ; Illegal function call
ER_OVERFLOW     = 6
ER_MEMORY       = 7             ; Out of memory
ER_LABEL        = 8             ; Label not defined
ER_SUBSCRIPT    = 9             ; Subscript out of range
ER_DUP          = 10            ; Duplicate definition
ER_DIV0         = 11            ; Division by zero
ER_DIRECT       = 12            ; Illegal in direct mode
ER_TYPE         = 13            ; Type mismatch
ER_STRING       = 14            ; Out of string space (a string past 64K)
ER_CONT         = 17            ; Cannot continue
ER_NOFUNC       = 18            ; Function not defined
ER_NORESUME     = 19            ; No RESUME
ER_RESUME       = 20            ; RESUME without error
ER_FORNEXT      = 26            ; FOR without NEXT
ER_WHILE        = 29            ; WHILE without WEND
ER_WEND         = 30            ; WEND without WHILE
ER_DUPLABEL     = 33            ; Duplicate label
ER_NOSUB        = 35            ; Subprogram not defined
ER_ARGS         = 37            ; Argument-count mismatch
ER_VARREQ       = 40            ; Variable required
ER_BADFILE      = 52            ; Bad file name or number
ER_NOTFOUND     = 53            ; File not found
ER_MODE         = 54            ; Bad file mode
ER_OPEN         = 55            ; File already open
ER_IO           = 57            ; Device I/O error
ER_EXISTS       = 58            ; File already exists
ER_FULL         = 61            ; Disk full
ER_PASTEND      = 62            ; Input past end of file
ER_BADNAME      = 64            ; Bad file name
ER_TOOMANY      = 67            ; Too many files
ER_DENIED       = 70            ; Permission denied
ER_PATH         = 76            ; Path not found
ER_BLOCK        = 80            ; (The Hydra's: a block's end without its start, or its start without its end)
ER_STACK        = 81            ; Out of stack space
ER_BREAK        = 82            ; (Ctrl-C: not an error a handler sees)
ER_TOOLONG      = 83            ; A line too long
ER_PARAM        = 84            ; Parameter type mismatch
ER_INSUB        = 85            ; Not in a procedure (EXIT SUB, SHARED ...)
ER_EXPECT       = 86            ; (A syntax error, with what was expected: ex_text)
ER_CONST        = 87            ; (A CONST's value that isn't a constant)
ER_FIELD        = 88            ; (A record's field that isn't its type's)
ER_PROMPT       = 89            ; (The prompt's command elsewhere: LIST, NEW, LOAD ... in a program)
INC_MAX         = 8             ; INCLUDE's files at most (one in another: INC_DEPTH deep), each its lines numbered
INC_DEPTH       = 4             ;   INC_LINE + its number * INC_STEP on (2047 lines at most), the program's below
INC_LINE        = $4000
INC_STEP        = $0800
EX_DIGITS       = 40            ; (E notation's digits at most, its text's room)
EX_MAX          = 200
EO_SIZE         = 80            ; (ERR$'s text at most)
ESC             = $1B
BEL             = $07
FMAX            = 8             ; Files open at once (#1 to #255)
FBSZ            = 32            ; Each one's read buffer (FMAX of them: 256 bytes)
FM_INPUT        = 1             ; A file's modes (OPEN's, a bit each)
FM_OUTPUT       = 2
FM_APPEND       = 4
FM_BINARY       = 8
G_RSV           = 8             ; Globals reserved first (gbase on): WINDOW's x1, y1, scales; the last point and the one
G_WX1           = 0             ;   being drawn, as the program has them (graphics, gfx.inc)
G_WY1           = 1
G_SX            = 2
G_SY            = 3
G_LLX           = 4
G_LLY           = 5
G_PLX           = 6
G_PLY           = 7

; .A, .X = an address (low, high)
.macro LDAX addr
            lda         #<(addr)
            ldx         #>(addr)
.endmacro

; A word copied: to = from
.macro MOVW to, from
            lda         from
            sta         to
            lda         from + 1
            sta         to + 1
.endmacro

; A word = a 16-bit constant
.macro LDW to, value
            lda         #<(value)
            sta         to
            lda         #>(value)
            sta         to + 1
.endmacro
