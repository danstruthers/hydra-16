;----------------------------------------------------------------------
;
;  Patrick Struthers - March 2026
;      - THANKS to AGSB for the starting point here....
;
;  The HyForth project starts here with AGSB's core Forth engine;
;  the engine will be moved to ROM and the heap will be copied to RAM
;  on cold start.
;
;  The ulitmate purpose of working this port through is to develop a
;  flexible and powerful operating system for the Hydra-16 of reasonable
;  efficiency and minimal memory footprint.  HyForth will grow and
;  shrink in RAM footprint according to context.
;
; ---------------------------------------------------------------------
;
.debuginfo
.setcpu "65C02"

;---------------------------------------------------------------------
; macros for dictionary, creates code as follows:
;
;   h_name:
;   .word  link_to_previous_entry
;   .byte  strlen(name) + flags
;   .byte  name
;   name:
;
; label for primitives
;
.macro makelabel arg1, arg2
.ident (.concat (arg1, arg2)):
.endmacro
;
; header for primitives
; the entry point for dictionary is h_~name~
; the entry point for code is ~name~
;
.macro def_word name, label, flag
makelabel "h_", label
.ident(.sprintf("H%04X", hcount + 1)):
  .word .ident (.sprintf ("H%04X", hlink))
hcount .set hcount + 1
hlink .set hcount
  .byte .strlen(name) + flag + 0 ; nice trick !
  .byte name
makelabel "", label
.endmacro
;
; header for a far word: its code is on BIOS ROM page A (farwords.s, the label ~name~ in scope FAR), and
; here it's a call to FARWORD, which runs it there
;
.macro def_far name, label
makelabel "h_", label
.ident(.sprintf("H%04X", hcount + 1)):
  .word .ident (.sprintf ("H%04X", hlink))
hcount .set hcount + 1
hlink .set hcount
  .byte .strlen(name)
  .byte name
makelabel "", label
    jsr FARWORD
    .word FAR::.ident(label)
.endmacro
;
; A library's words (a word set beyond the base language, e.g. LIBN_IO): the headers between lib_begin and
; lib_end go on that library's chain, not the base's.  A library's words can be in several places (each
; lib_begin goes on with its chain); the interpreter searches the chains of the libraries loaded (LIBSET),
; after the words defined in RAM and the base (LIB_NEXT).  LIB_HEADS, at the end, is each chain's head.
;
.macro lib_begin num
hsave .set hlink
hlink .set .ident(.sprintf("HL%d", num))
hlibnum .set num
.endmacro
;
.macro lib_end
.ident(.sprintf("HL%d", hlibnum)) .set hlink
hlink .set hsave
.endmacro
;---------------------------------------------------------------------
;  macros for PGS stuff
;
.macro WCRLF_np                ; no push of A
    PRINT_CRLF
.endmacro

.macro WCRLF
    pha
    WCRLF_np
    pla
.endmacro

.macro  WSEQ_np  strlbl
    phy
    ldy #0
:
    lda strlbl, y
    beq :+
    iny
    PRINT_CHAR
    bra :-
:
    WCRLF_np
    ply
.endmacro


.macro  WSEQ_raw  strlbl
    phy
    ldy #0
:
    lda strlbl, y
    beq :+
    iny
    PRINT_CHAR
    bra :-
:
    ply
.endmacro

.macro WSEQ  strlbl
    pha
    WSEQ_np  strlbl
    pla
.endmacro

;
;   ANSI screen stuff
; <rr>;<cc>f move cursor to rr,cc
; <cc>m for color/attributes
; 2J for clear screen
; H for 'home'
; <cc>[ABCD] screen moves
;
.macro ANSI b1,b2,b3,b4,b5,b6,b7
    pha
    PRINT_ANSI_ESC_SEQ b1, b2, b3, b4, b5, b6, b7
    pla
.endmacro

;---------------------------------------------------------------------
;  for error messages
;
.macro ERR_entry errmsg
    .byte <errmsg
    .byte >errmsg
    emcount .set emcount + 1
.endmacro

.macro WERR errptr
;.macro WERR
    .local werrloop, werrend
    WCRLF_np
    ldy #0
    lda (errptr),y
    sta TEMP0
    iny
    lda (errptr),y
    sta TEMP0+1
    ldy #0
werrloop:
    lda (TEMP0),y
    beq werrend
    iny
    PRINT_CHAR
    bra werrloop
werrend:                    ; (no CR LF: wrterror may add the reason first, then ends the line)
.endmacro

;---------------------------------------------------------------------
; variables for macros

hcount .set 0
hlink .set 0               ; the header the next one links to (its number: H0000 = none)
emcount .set 0             ; # of error messages set up

H0000 = 0

;---------------------------------------------------------------------
; The libraries: the word sets beyond the base language, each a chain of headers of its own (lib_begin),
; searched when its bit is in LIBSET.  Their names and what each needs are on page A (farwords.s: lib).
; LIB_BOOT is what HyForth loads at 'cold': everything the shell uses.
;
LIBN_IO     = 0            ; files and devices: open, read, mount, stty, ...
LIBN_FILES  = 1            ; the file and card commands: cd, ls, cp, cat, mkfs, fsck, ...
LIBN_SHELL  = 2            ; the shell: prompt, include, run, args, edit, echo; and a line's |, > and <,
                           ;   the prompt's format, and a word it doesn't know run as a program
LIBN_TASKS  = 3            ; tasks: shell, fg, kill, sleep, ps
LIBN_SOUND  = 4            ; the YM2151: sndinit, sndtest, sndstop, ywrite
LIBN_MEM    = 5            ; MMU memory: halloc, hfree, hlock, hunlock
LIBN_TOOLS  = 6            ; debugging and tests: dump, disasm, syscall, mmtest, hwtest
LIBN_TERM   = 7            ; the ANSI terminal: Acls, Ascr, Acol
LIB_COUNT   = 8
LIB_IO      = 1 << LIBN_IO ; (Their bits in LIBSET)
LIB_FILES   = 1 << LIBN_FILES
LIB_SHELL   = 1 << LIBN_SHELL
LIB_TASKS   = 1 << LIBN_TASKS
LIB_SOUND   = 1 << LIBN_SOUND
LIB_MEM     = 1 << LIBN_MEM
LIB_TOOLS   = 1 << LIBN_TOOLS
LIB_TERM    = 1 << LIBN_TERM
LIB_BOOT    = LIB_IO | LIB_FILES | LIB_SHELL | LIB_TASKS | LIB_SOUND | LIB_MEM | LIB_TOOLS | LIB_TERM
                           ; (loaded at 'cold': all of them, as the shell has always had)
;
; Libraries from files (lib: name.hyl, HyForth source, compiled into RAM on a chain of its own): up to
; RLIB_MAX, by slot.  LIBSET2's bits: the slots searched, and the base (LIB2_BASE, always), in that order,
; after the words defined in RAM and before the ROM libraries.  Their names and chains: hywords.s (RLIBNAME,
; LIB_HEADS2: the base's chain is its entry 7); their loading: farwords.s (lib)
RLIB_MAX    = 4
RLIB_NAMELEN = 12          ; (A name: 11 characters and a 0)
LIB2_BASE   = $80
.repeat LIB_COUNT, n       ; (Each library's chain so far: the number of its last header)
.ident(.sprintf("HL%d", n)) .set 0
.endrepeat

;---------------------------------------------------------------------
;               CONFIGURATION OPTIONS
;---------------------------------------------------------------------

; number conversion
numbers := 1      ; include DEC/BIN/HEX conversion

SINGLE := 1     ; single digits hard coded?

DEBUG := 1        ; enable inclusion of debug code

HYWORDS := 1      ; add in additional hardcoded words / logic

ANSIOK := 1         ; add ANSI screen stuff

YSOUND := 1        ; add sound support

;TXT2STACK := 1     ; use old TXTGET instead of new

;---------------------------------------------------------------------
;              for PGS hyforth stuff
;
; error codes
;
ERR_RPTR := $01    ; return stack pointer error
ERR_SPTR := $02    ; data stack pointer error
ERR_DIV0 := $03    ; divide by zero
ERR_MEM := $04     ; memory not-avail
ERR_UKW := $05     ; unknown word
ERR_SEC := $06     ; security error, ie dangerous address write
ERR_SYS := $07     ; return from system call error
ERR_IO := $08      ; IO error (the IO layer's error code: ioerr)
ERR_BRK := $09     ; break from the console (Ctrl-C)
  ; warnings
WRN_SEC := $86     ; security
WRN_MEM := $84     ; memory alloc
;
;---------------------------------------------------------------------
;                 CORE ENGINE CONSTANTS
;
CELL = 2         ; cell size, two bytes, 16-bit
FLAG_IMM = 1<<7  ; immediate flag
FLAG_COM = 1<<6  ; compiled flag
MAXSTR = 100

; terminal input buffer, forward
; getline, token, skip, scan, depends on page boundary
; INBUF = $0400  (see segment STACKS below)
; moves forwards
INBUF_end = $FD
HIST_SIZE = 255          ; the line editor's history (HIST: farwords.s, LINE_EDIT)

; data stacks
; moves backwards, push decreases before copy
DSEND = $7E

; return stack
; moves backwards, push decreases before copy
RTEND = $FE

; malloc stack
; moves backwards
MEMEND = $01FE
MEM_SZ = $04
MEM_MMU = $40             ; record type flag: the record is its own MMU block (large records)

; malloc records (see MALLOC): small ones go in an arena the MMU allocates at 'cold'; records of
; FORTH_LARGE_MIN bytes or more get their own MMU block
FORTH_ARENA_SIZE = $0800  ; 2K arena for small records
FORTH_LARGE_MIN = $0100   ; records this big or bigger get their own MMU block
FORTH_DICT_MARGIN = 2     ; pages above 'here' the MMU must keep free (see DICTCHK)

; malloc allocates DOWN from the top of the arena (MEMTOPV), set up at 'cold'

;----------------------------------------------------------------------
;       Look closely at hyforth.cfg and the output of ca65/ld65 after
;  a build; the RAM and ROM code is carefully arranged when the binary
;  is created, to make initialization easier and optimize RAM use.
;  The main program engine starts at $A000 in ROM and is only about 500 bytes
;  long.  Additional functions, then initialization and debug code,
;  then the dictionary follow in the binary, and these stay in ROM.
;
;       The dictionary has a particular structure, with 'bye' and 'abort'
;  at the beginning; there are some core words,
;  then contents of 'primitives.s', then 'hywords.s', followed by
;  the end of the dictionary with critical items such as 'fetch', 'store',
;  'immediate', 'compile, 'semis', 'exit, and ancillary stuff.
;
;       When $A003 is run from WozMon, a jump instruction to 'main' is
;  copied to $0600, followed by the dictionary, with 'exit' at the end.
;  HyForth is start by running '600R' or 'A000R'.
;
;----------------------------------------------------------------------
;                   ZERO PAGE USAGE
;----------------------------------------------------------------------
;   Task ZP (see TASK_ZP in include/kernel.inc): allocated top-down from $FF, so this
;   list runs from the top of ZP down.  The addresses are unchanged.
;
TASK_ZP_BEGIN
;
;          NXTTOK, BACKHEAP, TEMP5 - 7
;
TASK_ZP TEMP7, 2       ;    AUTOLOAD                           $FE
TASK_ZP TEMP6, 1       ;    WORDS, DIGCONT, TEXTGET            $FD
TASK_ZP TEMP5, 1       ;    WORDS, WFIND                       $FC
FFLAG = TEMP5          ; 'find flag' for wfind entry point
supprint = TEMP5       ; suppress printing in MEMCPY
TASK_ZP BACKHEAP, 2    ; hold 'here while compile              $FA
TASK_ZP NXTTOK, 2      ; next token in tib (INBUF)             $F8
;
;                    TEMP1 - 4
;
TASK_ZP TEMP4, 2       ; fourth  (two bytes)                   $F6
TASK_ZP TEMP3, 2       ; third  (two bytes)                    $F4
ramstart = TEMP3       ; used for COPYTORAM, MEMCPY
TASK_ZP TEMP2, 2       ; second                                $F2
endsoff = TEMP2        ; used for COPYTORAM, MEMCPY
TASK_ZP TEMP1, 2       ; first                                 $F0
mainoff = TEMP1        ; used for COPYTORAM, MEMCPY
;
;                   pointer registers
;
TASK_ZP WORKREG, 2     ; working register                      $EE
TASK_ZP INSTPTR, 2     ; instruction pointer                   $EC
TASK_ZP RTPTR, 2       ; return stack pointer                  $EA
TASK_ZP DSPTR, 2       ; data stack pointer                    $E8
;
;                   internal Forth
;
TASK_ZP NEXTHEAP, 2    ; next free cell in heap dictionary     $E6
TASK_ZP LASTHEAP, 2    ; last link cell                        $E4
TASK_ZP CURBUF, 2      ; CURBUF next free byte in TIB          $E2
TASK_ZP STATUS, 2      ; state at lsb, last size+flag at msb   $E0
;
;                   HyForth setup stuff
;
TASK_ZP ALFLAG, 1      ; autoload flag                         $DF
TASK_ZP RSEED, 4       ; random # seed                         $DB
TASK_ZP DIGBASE, 1     ; base for number conversion            $DA
TASK_ZP ERRPTR, 2      ; ptr to mitigation/message             $D8
TASK_ZP ERRFLAG, 1     ; error type, 0 = none                  $D7
TASK_ZP DFLAG, 1       ; debug flag                            $D6
TASK_ZP TIBEND, 2      ; pointer to end of TIB                 $D4
TASK_ZP TIB, 2         ; pointer to input buffer               $D2
TASK_ZP MEMLAST, 2     ;  malloc                               $D0
TASK_ZP MEMPTR, 2      ;   malloc                              $CE
TASK_ZP TEMP9, 2       ;   Imm                                 $CC
TASK_ZP TEMP8, 2       ;  Hstring macro                        $CA
TASK_ZP TEMP0, 2       ;  DUMPREG                              $C8
ZPSTART = TEMP0        ; CLEAR zeroes ZPSTART-$FF; the MMU variables below are set up by 'cold'
;
;                   MMU (not cleared by CLEAR)
;
TASK_ZP MEMBOT, 2      ; malloc arena bottom                   $C6
TASK_ZP MEMTOPV, 2     ; malloc arena top (MEMLAST starts here) $C4
TASK_ZP MEMHND, 1      ; MMU handle of the arena               $C3
TASK_ZP DICTLIM, 1     ; MMU page floor: 'here' stays below it $C2
TASK_ZP HLBANK, 1      ; RAM bank saved by hlock               $C1
TASK_ZP IOERR, 1       ; the last IO error (ioerr)             $C0
;
;                   Pipelines (see PIPECHK; set up by 'cold', not cleared by CLEAR)
;
TASK_ZP PIPEIN, 1      ; stdin saved while a pipeline runs ($FF: none) $BF
TASK_ZP BATCH, 1       ; <> 0: a pipeline's left side (a copy)  $BE
TASK_ZP CHILDSP, 1     ;   its stack pointer, to end the task  $BD
TASK_ZP PIPER, 1       ; the pipe being set up: read fd        $BC
TASK_ZP PIPEW, 1       ;   and write fd                        $BB
;
;                   Libraries (set up by 'cold', not cleared by CLEAR)
;
TASK_ZP LIBSET, 1      ; the libraries loaded (LIB_IO ...)     $BA
TASK_ZP LIBLEFT, 1     ;   those a search hasn't been through  $B9
TASK_ZP LIBSET2, 1     ; the RAM libraries searched (bit s: slot s), and the base (bit 7)  $B8
TASK_ZP LIBLEFT2, 1    ;   those a search hasn't been through  $B7
TASK_ZP_END
;
; *** $DO-$FF total usage in ZP, including TEMP vars ***
;
;   HYDRA-16 serial/read buffers at $200 and $300 so skip those
;
;----------------------------------------------------------------------
;                   FORTH STACKS
;----------------------------------------------------------------------
.segment "BUFFERS"
INBUF:
      .res 256
DS:                          ; data stack (S)
      .res 126
      .res 2
RT:                          ; return stack (R)
      .res 126
      .res 2
MEMSTK:                      ; memory manager
      .res 510
      .res 2
;
;
.segment "FORTH_ROM"              ; regular core
;
;
; ************ the real deal...
;

forth_main:
    jmp cold
    jmp COPYTORAM

; A bare Forth's task starts here ('forth', in the tasks library: TASK_RUN, ROM page 1): as a shell's
; (SHELL_MAIN: fds 0-2 on the console, HyForth's RAM), but 'cold' loads no libraries (BAREFLAG)
forth_bare_main:
    jsr IO_STD_OPEN
    jsr COPYTORAM
    inc BAREFLAG
    jmp cold

HYPROMPT:
    .byte $0D, $0A
    .byte "HF>"
    .byte 0
;
.ifdef DEBUG
WDISP:
    .byte $0D, $0A
    .byte "W="
    .byte 0
WATDISP:
    .byte "  [W]="
    .byte 0
.endif
;
;
;
cold:
    cld
    jsr CLEAR          ; zero out zero page, INBUF, DS, and RT
    stz BATCH          ; not a pipeline's copy
    stz IOERR          ; no IO error yet
    lda #LIB_BOOT      ; the base, and the libraries the shell needs; a bare Forth ('forth'): the base alone
    ldx BAREFLAG
    beq :+
    lda #0
:
    sta LIBSET
    lda #LIB2_BASE     ; (No RAM libraries: the base)
    sta LIBSET2
    lda #$FF
    sta PIPEIN         ; stdin not redirected
    lda #<fbreak       ; Ctrl-C: back to the prompt
    ldy #>fbreak
    ldx #1             ; (HyForth's ROM page)
    jsr TASK_SET_BREAK

; Forth owns this task's MMU memory: free anything left from before (only the MMU area: typed-ahead
; input in the message rings is kept), then get the malloc arena
    jsr MM_TASK_INIT
    stz MEMBOT                 ; no arena (every small malloc fails) unless MM_ALLOC works
    stz MEMBOT+1
    stz MEMTOPV
    stz MEMTOPV+1
    stz DICTLIM
    lda #<FORTH_ARENA_SIZE
    ldy #>FORTH_ARENA_SIZE
    ldx #0
    jsr MM_ALLOC               ; whole pages at the top of task RAM
    bcs NOARENA
    sta MEMHND
    jsr MM_LOCK                ; .A.Y = address (page blocks don't move), .X = RAM bank
    sta MEMBOT
    sty MEMBOT+1
    lda MEMHND
    jsr MM_UNLOCK
    lda MEMBOT
    clc
    adc #<FORTH_ARENA_SIZE
    sta MEMTOPV
    lda MEMBOT+1
    adc #>FORTH_ARENA_SIZE
    sta MEMTOPV+1
NOARENA:

warm:
; link list of headers: none in RAM yet (the base's, and the libraries': LIB_NEXT)
    stz LASTHEAP + 1
    stz LASTHEAP

; next heap free cell
    lda #>FORTH_BSS_END + 1    ; (after the buffers that follow the RAM image)
    sta NEXTHEAP + 1
    stz NEXTHEAP
    stz ERRFLAG                ; clear ERROR flag


    ldy #>(MEMSTK+MEMEND)       ; initialize memory manager area
    sty MEMPTR + 1
    ldy #<(MEMSTK+MEMEND)
    sty MEMPTR
    ldy MEMTOPV                 ; malloc records grow down from the top of the arena
    sty MEMLAST
    ldy MEMTOPV+1
    sty MEMLAST+1
    jsr DICTCHK                 ; keep the MMU out of the pages above 'here'

    bra reset
;
; A break from the console (Ctrl-C; see TASK_SET_BREAK in 'cold'): the task comes here, with the stack
; pointer it had at 'cold', wherever it was: back to the prompt, as for an error
fbreak:
    lda #ERR_BRK
    sta ERRFLAG
    jmp abort
;---------------------------------------------------------------------
; various reinitialization points
;
reset:
    ldy #INBUF_end
    sty TIBEND
    ldy #<INBUF
    sty TIB
    ldy #>INBUF
    sty TIB+1
    sty TIBEND+1
    sty CURBUF+1
    sty NXTTOK+1

    ldy #>DS                     ; DS and RT are now half page each
    sty DSPTR + 1
    ldy #>RT
    sty RTPTR + 1

    lda #1                       ; DEBUG OFF by default
    sta DFLAG
    stz ALFLAG                   ; autoload flag OFF

abort:                            ; clear DS
    ldy #<DSEND
    sty DSPTR

errrtn:                          ; return from error
quit:                             ; clear RT
    ldy #<RTEND
    sty RTPTR

    lda ERRFLAG                  ; (an error while a script is read stops it, and the ones that include it)
    pha
    jsr wrterror                 ; print any error messages
    pla
    beq ERRNOINC
    jsr INCABORT
ERRNOINC:
    ldy #0          ; reset INBUF
    lda #0
    sta (TIB),y     ; clear INBUF stuff
    stz CURBUF    ; clear cursor  (pointer into INBUF)
    stz STATUS    ; status is 'interpret' == \0

    .byte $2c       ; mask next two bytes, nice trick !
;---------------------------------------------------------------------
; the outer loop

resolvept:
    .word okey
;---------------------------------------------------------------------
okey:               ; well shit, I hope this is easy....
resolve:           ; get a token
    jsr token      ; then just process the regular way
.ifdef DEBUG
    lda DFLAG                 ; DEBUG
    bne RVPSKIP
    WCRLF_np
    lda #'P'
    PRINT_CHAR            ; DEBUG
.endif

RVPSKIP:

RESFIND:                ; load last or 'latest' word on heap
    lda LASTHEAP + 1
    sta TEMP2 + 1
    lda LASTHEAP
    sta TEMP2
    lda LIBSET              ; (then the RAM libraries, the base, and the libraries: LIB_NEXT)
    sta LIBLEFT
    lda LIBSET2
    sta LIBLEFT2

RESLOOP:              ; lsb linked list
    lda TEMP2
    sta WORKREG             ; so 'last' -> W
    ora TEMP2+1             ; only zero if both are zero
    bne RESEACH              ; PGS - did he forget this?
    jsr LIB_NEXT            ; the end of a chain: the next library's
    bcc RESLOOP

WORDNOTFOUND:
    lda ERRFLAG            ; keep an error already raised (e.g. out of memory for a q^...^ string)
    bne WNFERR
    lda STATUS             ; interpreting: the program of that name?  (name.hyx or name.hys: RUNNAME,
    bne WNFUKW             ;   with the shell's library)
    jsr RUNNAME
    bcc WNFRAN
    cmp #ERR_IO_NOT_FOUND
    beq WNFUKW
    jmp IOFAIL
WNFRAN:
    jmp resolve
WNFUKW:
    lda #ERR_UKW           ; UNKNOWN WORD error
    sta ERRFLAG
WNFERR:
    jmp errrtn ; end of dictionary, no more words to search, abort
;
DICTFULL:                  ; the dictionary would grow into memory the MMU has allocated
    lda #ERR_MEM
    sta ERRFLAG
    jmp errrtn

RESEACH:                        ; msb linked list
    lda TEMP2 + 1
    sta WORKREG + 1           ; update next link

    ldx #WORKREG
    ldy #TEMP2
    jsr copyfrom                  ; W += 2, now pointing at size/flag byte from 'here'
    ldy #0              ; compare words
    lda (WORKREG), y    ; save the flag, first byte is (size and flag)
    sta STATUS + 1
            ;; *** start of mod for bit check
    and #$3F            ; mask off flags
    sec
    sbc (NXTTOK), y    ; compare lengths
    bne RESLOOP
    iny
; compare chars
RESEQUAL:
    lda (NXTTOK), y
    cmp #ASCII_SPACE            ; space ends
    beq RESDONE
    sec                 ; verify
    sbc (WORKREG), y
    asl                 ; clean 7-bit ascii
    bne RESLOOP
    iny                 ; get next char
    bne RESEQUAL

RESDONE:
    tya                ; increment W by y, W will point at CFA?
    jsr addwx

eval:
; executing ? if status = 0
    lda STATUS
    beq execute
;
; falls thru on compile, but...
; immediate ? if status+1 < 0 (bit seven set)
    lda STATUS + 1
    bmi immediate

compile:          ; otherwise compile
.ifdef DEBUG
    lda DFLAG          ; DEBUG, print C if here
    bne CMPSKIP
    WCRLF_np
    lda #'C'
    PRINT_CHAR
CMPSKIP:
.endif
    jsr DICTCHK         ; room for the dictionary to grow?
    bcs CMPROOM
    jmp DICTFULL
CMPROOM:
    jsr wcomma          ; copy W into NEXTHEAP ('here'), increment NEXTHEAP
    bcs immediate
    jmp resolve         ; if not 'immediate' go on to next token
;
immediate:
execute:

.ifdef DEBUG
    lda DFLAG         ; DEBUG, print E if here
    bne EXESKIP
    WCRLF_np
    lda #'E'
    PRINT_CHAR
EXESKIP:
.endif
    lda #>resolvept     ; set up INSTPTR to run,
    sta INSTPTR + 1     ; or return to interpreter.
    lda #<resolvept
    sta INSTPTR
    jmp pick             ; almost done, 'next' and either ENTER or EXEC

;-----------------------START PROCESSING INPUT-----------------------
try:
    lda (TIB), y                   ; index is in y
    beq getline    ; if \0  - get a line if pointing at 0
    iny
    eor #ASCII_SPACE    ; return 0 in  A if a space
    rts

;--------------------GET AN INPUT LINE ENDING WITH CR/LF ------------
getline:   ; drop rts of try, fall through to 'token'
    pla
    pla
    jsr LINE_START       ; the shell library's: the last line's > < and pipe undone; boot.hys (farwords.s)
    lda BATCH            ; a copy of the shell (a pipeline's left side, or run's for a script): all done,
    beq GLAUTO
    lda INCDEPTH         ;   once the scripts it reads are: end the task, with the status (farwords.s)
    bne GLAUTO
    jmp LINE_EXITS
GLAUTO:
;
;   DO AUTOLOAD HERE
;      load a space, then copy next line to buffer
;      calc y (length + 1) jump to GETLNEND
;
    lda ALFLAG
    beq GLNORMAL
    jsr ALOADTIB
    jmp GETLNSKIPCRLF

GLNORMAL:
    stz ECHOCR           ; (A line read from a script: no prompt, and no CR LF after it)
    lda INCDEPTH
    beq GLCONS
    jsr INCCOUNT         ; (its line number, for an error message)
    bra GLNOPROMPT
GLCONS:
    lda IO_FD_SERVER     ; the prompt (the shell's, page 7), unless input is a file or a pipe.  (No
    cmp #IO_FD_CLOSED    ;   fd 0 at all: the console, read directly)
    beq GLPROMPT
    lda IO_FD_FLAGS      ; (fd 0's flags: IO_FDF_CONS, the console)
    and #IO_FDF_CONS
    beq GLNOPROMPT
GLPROMPT:
    inc ECHOCR
    jsr LINE_PROMPT      ; (the shell's, or a plain one; then the line, edited, if fd 0 is the console: farwords.s)
    bcc GETLNEND         ; (.Y = its length + 1)
GLNOPROMPT:
;
    ldy #0   ; leave the first
GETLOOP:
    sta (TIB), y  ; dummy store on first pass, overwritten
    iny
    cpy TIBEND
    beq GETLNEND
    cpy #$FF
    bne GETREADLOOP
    ldy #1
GETREADLOOP:
    jsr GET_CHAR      ; (sleeps until a key comes in)
    bcs GETGOT
    lda INCDEPTH      ; nothing: the end of a script being read (the console: wait on)
    beq GLEOF
    jsr INCEND        ; stdin back to what it was, and what was read of the last line is the line
    bra GETLNEND
GLEOF:
    jsr LINE_EOF      ; the end of stdin: a file or a pipe ends the task (the console: nothing)
    bra GETREADLOOP
GETGOT:
    cmp #ASCII_TAB    ; (a script's tabs are spaces, and its lines may end with LF)
    bne GETNOTAB
    lda #ASCII_SPACE
GETNOTAB:
    cmp #ASCII_LF
    bne GETNOTLF
    cpy #1            ; an LF: the line's end; but not the LF of a CR LF (at the line's start, after
    bne GETLFEND      ;   a line that ended with CR)
    lda LASTCR
    beq GETLFEND
    stz LASTCR
    bra GETREADLOOP
GETLFEND:
    stz LASTCR
    bra GETLNEND
GETNOTLF:
    cmp #ASCII_CR
    bne GETNOTCR
    sta LASTCR
    bra GETLNEND
GETNOTCR:
    cmp #ASCII_BACKSPACE         ; handle backspace
    bne GETLOOP
    cpy #2
    bcc GETBSNONE     ; nothing typed yet: nothing to erase
    dey
    dey
    lda (TIB), y      ; make sure prev char not overwritten
    bra GETLOOP
GETBSNONE:            ; (/dev/cons's echo erased the prompt's last character: put it back)
    lda PROMPTLAST
    PRINT_CHAR
    bra GETREADLOOP
GETLNEND:                ; clear all if y eq \0
    lda ECHOCR        ; (the console's line: a new line after it)
    beq GETLNSKIPCRLF
    PRINT_CRLF
GETLNSKIPCRLF:          ; SKIP to here if don't want CRLF
    lda #ASCII_SPACE
    phy
    ldy #0
    sta (TIB), y       ; start with space
    ply
    sta (TIB), y        ; ends with space
    lda #0            ; mark eol with 0
    iny
    sta (TIB), y
    dey
; start it
    sta CURBUF
    jsr LINE_READ        ; the shell library's: a pipeline started, > >> < set up (farwords.s)
    bcc token
GETLNFAIL:
    ply                  ; (they can't be: drop the return to 'resolve', and the error)
    ply
    jmp IOFAIL

;---------------------------------------------------------------------
; in place every token,
; the counter is placed at last space before word
; no rewinds
token:
    ldy CURBUF   ; last position on INBUF

TOKENSKIP:   ; skip spaces
    jsr try
    beq TOKENSKIP
    dey   ; keep y == <start of input word> + 1
    sty NXTTOK

TOKENSCAN:  ; scan spaces
    jsr try
    bne TOKENSCAN
    dey   ; keep y == <end of input word> + 1
    sty CURBUF

TOKENDONE:  ; find size and store it;
    tya
    sec
    sbc NXTTOK
    ldy NXTTOK    ; keep it
    dey
    sta (TIB), y  ; store size for counted string
    sty NXTTOK
    ;
    ;  During interpretive mode at least...do number and string capture here, before
    ;     looking at word list; will be pushed on stack.
    ;  This SHOULD have a general digit converter (any base up to 16); return C = 1 if no conversion
    ;   if SINGLE is defined, this will skip single digit numbers and use hardcoded ones.
    ;
    ;  Following check for #'s, check for quoted inline txt a la 'q^....^'
    ;
.ifdef numbers
    ldy #0
    lda (NXTTOK),y
    tax                ; store size in X, pass to conversion
    jsr DIGCONVT
    bcs CHKFERTXT       ; if some error in conversion, skip and continue processing
    lda STATUS          ; compiling: the number goes into the definition (LITCOMPILE)
    beq TOKCLR0
    jsr LITCOMPILE
    bra TOKCLR0
;
; A number in a definition (DIGCONVT pushed it): compiled as 'lit' and the number, so it's pushed when the
; word runs, as 'lit [ n , ]' writes it by hand.  (DICTCHK's margin has room: the next word compiled checks it)
.pushseg
.segment "FORTH_TOP"
LITCOMPILE:
    jsr spull_0
    lda #<literal
    sta WORKREG
    lda #>literal
    sta WORKREG + 1
    jsr wcomma
    ldy #TEMP1
    jmp comma
.popseg
.endif  ; 'numbers'
CHKFERTXT:
    jsr TEXTGET         ; returns length +4 in X
    bcs TOKENEND
    ldy #0
    lda #ASCII_SPACE
    jmp TOKCLR
TOKCLR0:
    ldy #0
    lda (NXTTOK),y     ; load length again
    tax
    inx
    lda #ASCII_SPACE          ; copy spaces over entire converted string
TOKCLR:
    sta (NXTTOK),y
    iny
    dex
    bne TOKCLR
TOKNEXT:
    jmp token               ; and use 'token' to rebuild input buffer w/o converted #


TOKENEND:
    clc     ; clean - setup token
    rts
;
;--------------------UTILITIES----------------------------------------
;    A whole bunch of helper functions;
;  wcomma / comma increment WORKREG (or another reg) and NEXTHEAP ('here')
;  while copying into NEXTHEAP, uses incwx.
;
;  copyinto / copyfrom copy AND increment/decrement
;  spush/rpush - pushes something onto stacks indexed on ZP by Y (increment using addwx/incwx)
;  spull/rpull - pulls from stacks, copies into ZP indexed by Y (decrement builtin)
;
;  addwx/incwx/decwx do some of the inc/dec-rementing duties
;
;---------------------------------------------------------------------
;
;   COMMA allocates memory at top of heap for 'other stuff'
;   and makes sure heap and working reg pointers are updated.
;
; heap linked list (moves forward)
;
wcomma:
    ldy #WORKREG                  ; copy addr at WORKREG, change addr fld NEXTHEAP points to
comma:
    ldx #NEXTHEAP                 ; Y has source of address, change addr fld NEXTHEAP points to
    ; FALL THROUGH - copyinto, then rts after second incwx
;---------------------------------------------------------------------
; from a page zero address indexed by Y
; into a page zero indirect address indexed by X
;
copyinto:
    lda 0, y
    sta (0, x)
    jsr incwx
    lda 1, y
    sta (0, x)
    jmp incwx                        ; incwx ends with rts!
;---------------------------------------------------------------------
;
; generics - PUSH, PULL, incwx/addwx, copyfrom
;
;------------------------PUSH a cell--------------------------------
spush_2:
    ldy #TEMP3       ; push TEMP3 on top
    jmp spush
spush_1:
    ldy #TEMP2       ; push TEMP2 on top
    jmp spush
spush_0:             ; push TEMP1 to stack, probably top of stack
    ldy #TEMP1
     ; FALL THROUGH
;---------------------------------------------------------------------
; PUSH a cell
; from a page zero address indexed by Y
; into a page zero indirect address indexed by X
spush:
    ldx #DSPTR
    lda DSPTR
    cmp #<DS                ; ditto
    beq ptrerr_s              ; ditto
    jmp push
rpush:
    ldx #RTPTR
    lda RTPTR               ; ditto
    cmp #<RT                ; ditto
    beq ptrerr_r

    ; FALL THROUGH
;---------------------------------------------------------------------
; classic stack backwards
push:
    jsr decwx
    lda 1, y
    sta (0, x)
    jsr decwx
    lda 0, y
    sta (0, x)
    rts
;
;                      pointer error (DS or RT)
ptrerr_r:                      ; pop jsr off stack, throw error
    lda #ERR_RPTR
    bra ptrerr_cont
ptrerr_s:
    lda #ERR_SPTR
ptrerr_cont:
    sta ERRFLAG
    pla
    pla
    jmp errrtn
;
;---------------- PULL a cell, with convenience for TEMP 1/2/3 -----
;
spull_2:
    ldy #TEMP3              ; pull TEMP3 from top
    jmp spull

spull_1:                    ; pull TEMP2 from top
    ldy #TEMP2
    jmp spull

spull_0:
    ldy #TEMP1             ; pull TEMP1 from top of DS
;
;  FALL THROUGH
;
; PULL a cell
; from a page zero indirect address indexed by X
; into a page zero address indexed by y
;
spull:
    ldx #DSPTR
    lda DSPTR         ; pointer bounds checking
    cmp #DSEND        ; ditto
    beq ptrerr_s      ; ditto
    jmp pull          ; pull includes rts from incwx so....
rpull:                ;
    ldx #RTPTR
    lda RTPTR        ; pointer bounds checking
    cmp #RTEND        ; ditto
    beq ptrerr_r        ; ditto
;
;  FALL THROUGH
;---------------------------------------------------------------------
;
; from a page zero indirect address indexed by X
; into a page zero address indexed by y
pull:
copyfrom:
    lda (0, x)
    sta 0, y
    jsr incwx      ; NOTE:  not a jmp, returns here.
    lda (0, x)
    sta 1, y
 ;
 ;  FALL THROUGH
;---------------------------------------------------------------------
; increment a word in page zero. offset by X
;
;   Usage: ldx #<pointer name> and then jsr incwx/addwx
;
;   THESE functions are SOLELY intended to increment/decrement
;   zeropage pointers, and nothing else.  Offsets indexed by
;   X are INTO the zeropage space, not relative to a specific
;   pointer location.
;
incwx:
    lda #01
;---------------------------------------------------------------------
; add a byte in A to a word in page zero. offset by X
addwx:
    clc
    adc 0, x
    sta 0, x
    bcc addwx_end
    inc 1, x
    clc      ; keep carry clean.
             ; our convention is that functions SET the carry for positive results,
             ; clear carry for negative or neutral ones.
addwx_end:
    rts

;---------------------------------------------------------------------
;          decwx, heap moves
; decrement a word in page zero. offset by X
;
;   Usage: ldx #<pointer name> and then jsr decwx
;
;   THESE functions are SOLELY intended to increment/decrement
;   zeropage pointers, and nothing else.  Offsets indexed by
;   X are INTO the zeropage space, not relative to a specific
;   pointer location.
;
decwx:
    lda 0, x
    bne decwx_end
    dec 1, x
decwx_end:
    dec 0, x
    rts
;
;---------------------------------------------------------------------
; A far word (def_far): its code is `jsr FARWORD`, then the address of its code on BIOS ROM page A
; (farwords.s), which runs there (through the gate FW_CALL) and comes back here: on to 'next', or with C = 1
; to 'errrtn' (ERRFLAG says what the error is)
;
FARWORD:
    pla                           ; (the address of the address, - 1)
    sta ZP_FAR_VEC
    pla
    sta ZP_FAR_VEC + 1
    ldy #1
    lda (ZP_FAR_VEC), y
    pha
    iny
    lda (ZP_FAR_VEC), y
    tay
    pla                           ; .A.Y = the code
    jsr FW_CALL
    bcs FARWORD_ERR
    jmp next
FARWORD_ERR:
    jmp errrtn
;
; An IO error, .A: 'ioerr' has it, and !IO ERR!
IOFAIL:
    sta IOERR
    lda #ERR_IO
    sta ERRFLAG
    jmp errrtn
;
;
;
ENGINEEND:
;                           END OF CORE ENGINE
;----------------------------------------------------------------------
;
;                        DICTIONARY and ADDITIONS
;
;    upper.s -- error messaging, CLEAR on start, debug, utilities
;------------------------------------------------------------------------
;
upper_ram:
   .include "upper.s"
UPPER_END:
;
;
YSOUND_START:
;.ifdef YSOUND
;   .include "sound_os.s"
;.endif
YSOUND_END:
;
;
ROMCODEEND:                         ; end of all code
;
;  end of hyforth.s
;
;-----------------------------------------------------------------------
    .res 16                    ; just a visible buffer in binary
                               ; to make easier to identify different
                               ; code segments.
;-----------------------------------------------------------------------
;                  BELOW, ONLY HYFORTH'S VARIABLES END UP IN RAM
;-----------------------------------------------------------------------
.segment "FORTH_PAGED_ROM"
;------------------------------------------------------------------------
;
;    hyf_rom.s -- COPYTORAM
;
.include "hyf_rom.s"
;
;
COPYSTART := $A300              ; marks beginning of copy in ROM space (bank 0; $A000-$A1FF: the ROM disk's table)
;
;   The variables (segment FORTH_DATA: the words' data that changes, and what other ROM pages read, as
;   the shell's page 7 does) are copied to RAM at RAMSTART ($0800), up to 'ends', by COPYTORAM.  The code and
;   the built-in words' headers (segment FORTH_CORE) run from ROM, on page 1, as the engine above does; the
;   dictionary (user words) grows in RAM, after the variables and the buffers that follow them.
;
.segment "FORTH_DATA"
RAMSTART:
;
;---------------------------------------------------------------------
;        farwords.s -- the words whose code is on BIOS ROM page A (scope FAR), and the disassembler
;
.include "farwords.s"
;
.segment "FORTH_CORE"
;---------------------------------------------------------------------
;        primitives.s -- original AGSB hardcoded dictionary
;
primitives:
.include "primitives.s"
;
;
;------------ CRITICAL CORE PRIMITIVES (AGSB and PGS) ----------------
;
;   COMPILE (:), FINISH (;), FETCH (@), STORE (!)
;      including:  keeps, this, next, finish
;       ...and other stuff.
;
;---------------------------------------------------------------------
; ( a -- ) execute a jump to a reference at top of data stack
def_word "exec", "exec", 0
    jsr spull_0
    jmp (TEMP1)        ; assumes an address on top of DS

;---------------------------------------------------------------------
; ( -- ) execute a jump to a reference at IP
def_word ":$", "docode", 0
    jmp (INSTPTR)                 ;  DOCODE (thus the ':')

;---------------------------------------------------------------------
; ( -- ) execute a jump to next
def_word ";$", "donext", 0        ;  'next' (thus the ';')
    jmp next
;---------------------------------------------------------------------
;
; ( w a -- ) ; [a] = w    (w is word, a is an address)
def_word "!", "store", 0
storew:
    jsr spull_1             ; get address, store in TEMP2
    jsr spull_0             ; get data, store in TEMP1
    ldx #TEMP2              ;  [a]
    ldy #TEMP1              ;   w
    jsr copyinto            ; copy TEMP2 stuff to addr in TEMP1 (opposite of @)..
    jmp next                ;            ...(see below)
;
;--------------------------------------------FETCH--------------------
; ( a -- w ) ; w = [a]
def_word "@", "fetch", 0      ; replace addr of data on top of DS, with data pointed to
fetchw:
    jsr spull_0             ; get addr from DS
    ldx #TEMP1
    ldy #TEMP2
    jsr copyfrom            ; copies data from [TEMP1] => TEMP2
;---------------------------------------------------------------------
;            NEXT entry point for many AGSB primitives
;---------------------------------------------------------------------
copys:                      ; copy from cell at y (zp) to TEMP1
    lda 0, y
    sta TEMP1
    lda 1, y
keeps:                      ; saves bytes since have to get here anyway
    sta TEMP1+1
this:                       ; same as above
    jsr spush_0             ; then push back on stack
    jmp next
;
;-----------------------IMMEDIATE, '[', ']', ','-----------------------
def_word "I", "Imm", 0
wimm:                        ; jmp here if proccessing a compiled
    lda LASTHEAP+1           ;   word that needs to run 'immediate'.
    sta TEMP9+1
    beq IMMNONE              ; (no word defined yet: nothing to do)
    lda LASTHEAP             ; get addr of 'last' compiled word, copy to TEMP4, add 2
    clc
    adc #2                   ; ..to find where length byte is...
    sta TEMP9
    bcc IMMSKIP
    inc TEMP9+1
IMMSKIP:
    ldy #0
    lda (TEMP9),y
    ora #$80                 ; ...set bit 7 and store
    sta (TEMP9),y
IMMNONE:
    jmp next
;
def_word "[", "leftbrack", FLAG_IMM       ; switch to 'interpret'
    stz STATUS
    jmp next
;
def_word "]", "rtbrack", 0               ; switch back to 'compile'
    lda #1
    sta STATUS
    jmp next
;
;
                                         ; NOTE: 'lit' or similar will have pushed a value
                                         ; or address onto stack; 'comma' stores it inline in word
                                         ;
def_word ",", "xcomma", 0                ; pull data from top of stack, store at 'here', adjust
    jsr spull_0                          ; 'here' to point at next cell
    ldy #0
    lda TEMP1
    sta (NEXTHEAP),y                     ; POP DS TO TEMP1, [here] = TEMP1
    iny
    lda TEMP1+1
    sta (NEXTHEAP),y
    ldx #NEXTHEAP                        ; here += 2
    lda #2
    jsr addwx
    jmp next

;--------------------------------------------SEMIS-----------------
def_word ";", "semis",  FLAG_IMM
    lda BACKHEAP
    sta LASTHEAP                ; bring back BACKHEAP to LASTHEAP
    lda BACKHEAP + 1
    sta LASTHEAP + 1

    stz STATUS                  ; set status to 'interpret' (presumably from 'compile')

finish:                         ; compiled words must end with exit
    lda #<exit
    sta WORKREG
    lda #>exit
    sta WORKREG + 1
    jsr wcomma                  ; change NEXTHEAP to point to addr of 'exit',
                                ; and make sure is last entry in code table for word...
                                ; as all good Forth compiled words should do.
    jmp next
;
;
;------------------------------------------COMPILE------------------
def_word ":", "colon", 0
    jsr DICTCHK                 ; room for the dictionary to grow?
    bcs COLONROOM
    jmp DICTFULL
COLONROOM:
    lda NEXTHEAP
    sta BACKHEAP                ; backup NEXTHEAP to BACKHEAP
    lda NEXTHEAP + 1
    sta BACKHEAP + 1

    lda #1                      ; set status to 'compile'
    sta STATUS

COMPHEADER:
; copy LASTHEAP into (NEXTHEAP)
    ldy #LASTHEAP
    jsr comma                    ; change NEXTHEAP to point to LASTHEAP  ('here' <= 'last')
    jsr token                    ; get first token, the name of new word
    ldy #0                       ; copy it to heap: length and name
                                 ; code field comes with later proc
COMPLOOP:
    lda (NXTTOK), y
    cmp #ASCII_SPACE
    beq COMPEND
    cpy #0                       ; if this is the length field...
    bne COMCOPY
    ora #$40                     ; set bit six for 'compiled' word
COMCOPY:
    sta (NEXTHEAP), y
    iny
    bne COMPLOOP
COMPEND:
    tya                          ; and update NEXTHEAP  :  'here' incremented by length
    ldx #(NEXTHEAP)
    jsr addwx                   ; 'here' now at CFA

;~~~~~~~~ all done....
    jmp next                     ; and then see below; compiled word will continue until ';'
;
;---------------------ADD IN EXTRA HARDCODED LOGIC / NUMERALS / ETC--
.ifdef HYWORDS
   .include "hywords.s"
.endif
;---------------------------------------------------------------------
; Thread Code Engine
;
;   INSTPTR is IP, WORKREG is W
;
;     unnest, next, pick, nest, and jump
;
;   nest aka ENTER or DOCOL  (do colon?)
;   unnest aka EXIT or semis?
;
;---------------------------------------------------------------------
; ( -- )
def_word "exit", "exit", 0
    jmp EXIT
;
;---------------------------------------------------------------------
; The libraries' chains (after the words in RAM and the base's: RESFIND, 'words').  The next one to search:
; the lowest in LIBLEFT, which is taken out of it.  OUT: C = 0: TEMP2 = its head; C = 1: none left.
; Modifies: .A, .X
LIB_NEXT:
    lda LIBLEFT2         ; First the RAM libraries (by slot) and the base (LIBSET2, LIB_HEADS2 in RAM) ...
    beq LNROM
    jsr LNBITX
    lda LIB_HEADS2,x
    sta TEMP2
    lda LIB_HEADS2+1,x
    sta TEMP2+1
    lda LIBLEFT2         ; (its bit, the lowest, cleared)
    dec a
    and LIBLEFT2
    sta LIBLEFT2
    clc
    rts
LNROM:
    lda LIBLEFT          ;   then the ROM libraries (LIBSET, LIB_HEADS)
    sec
    beq LNNONE
    jsr LNBITX
    lda LIB_HEADS,x
    sta TEMP2
    lda LIB_HEADS+1,x
    sta TEMP2+1
    lda LIBLEFT          ; (its bit, the lowest, cleared)
    dec a
    and LIBLEFT
    sta LIBLEFT
    clc
LNNONE:
    rts
LNBITX:                  ; (.X = the number of .A's lowest bit (not 0) x 2)
    ldx #0
LNBIT:
    lsr
    bcs LNFOUND
    inx
    inx
    bra LNBIT
LNFOUND:
    rts
;
; Each library's chain: its last header (0: none), by LIBN_
LIB_HEADS:
.repeat LIB_COUNT, n
    .word .ident(.sprintf("H%04X", .ident(.sprintf("HL%d", n))))
.endrepeat

; mark end of ROM IMAGE
;
;.byte "\CODEEND/"
;.byte 0
;-----------------------------------------------------------------------
; BEWARE, MUST BE AT END! MINIMAL THREAD CODE DEPENDS ON IT!
;
.segment "FORTH_DATA"
ends:                            ; end marker of the variables (COPYTORAM)
;
; Buffers in the RAM after the variables (COPYTORAM copies them up to 'ends'), before the dictionary:
; nothing in the paged ROM for them.  (Their contents are whatever was there.)
ARGBUF      = ends                  ; a parsing word's argument (ARGGET)
SHBUF       = ARGBUF + ARGBUF_SIZE  ; a path: the current directory, a name being made (shell/shell.s)
SHBUF2      = SHBUF + 64            ; another
SDCMD       = SHBUF2 + 64           ; a card's ctl command (mkfs, fsck ...)
SHOWBUF     = SDCMD + SDCMD_SIZE    ; a file being shown (FSHOW)
INCFD       = SHOWBUF + SHOWBUF_SIZE    ; include: each script's reader's saved stdin (INC_MAX fds) ...
INCLINE     = INCFD + INC_MAX           ;   and the line it's on (INC_MAX x 2)
SHLABEL     = INCLINE + INC_MAX * 2     ; the prompt's %l: card LBLCARD's HydraFS label (32 bytes)
SHOPT       = SHLABEL + 32              ; mkfs: HFS_FMT_FULL (mkfs-full), HFS_FMT_PART (mkfs-part); ls: <> 0: -l ...
SHSIZE      = SHOPT + 1                 ;   and the volume's size in megabytes (0: the whole card) (2)
ARGREC      = SHSIZE + 2                ; A program's arguments (ARGREST), as a string record for args: its
ARGLINE     = ARGREC + 3                ;   3-byte header, then the text (ARGLINE_SIZE bytes)
FORTH_BSS_END = ARGLINE + ARGLINE_SIZE
INC_MAX     = 4                     ; Scripts include can nest
;
;-----------------------------------------------------------------------
;
;

;-----------------------------------------------------------------------
;                            TRAINING DATA
;
; include training data

.byte "***HEAPEND***"
;.org $C000
    .byte "FTRAIN"
    .byte 0,0
.include "ftrain.s"
;
;                            BINARY LOAD
;
    .res 16
    .byte "BLOAD"
    .byte 0,0
.include "bload.s"
;
;  end of hyforth.s
;
