; ****************************************************************************
; forth - HyForth, rebuilt (docs/reimplementation-from-scratch.md, §16): a Forth 2012 system, a program run in place
; from its paged ROM module.  `forth` at rc's prompt starts it; `forth <file` runs a file of source (its lines from
; stdin, as typed ones).  This module is the core: the Core word set, and INCLUDED, INCLUDE, REQUIRED, REQUIRE, \ and
; BYE, to load the rest.  The other word sets are libraries, pre-compiled (forthlib/NAME.s, tools/forthlib.js:
; /lib/forth/NAME.fl), loaded into the dictionary as INCLUDED or REQUIRE names them: Core Extension, Exception, File
; Access, Facility, String, Search-Order and Programming-Tools, with their extensions (but the editor's, the
; assembler's and EKEY's), Double-Number (a few words), and the Hydra's (a sys- word for each system call a program
; makes, made from the specification; SH and RUN, banks and segments, ARGC and ARG).  As it starts, forth INCLUDEs
; /lib/forth/startup.fs, if there is one (the ROM disk's: REQUIRE coreext.fl exception.fl file.fl tools.fl).  Ctrl-C
; (a note) is THROW -28 at the next word, loop or wait.  `forth file.fs [argument ...]` runs a script (the file
; INCLUDED, then the end: code 1 after an error); a file's first line #!... is skipped; a name INCLUDED that isn't
; there, with no / in it, is /lib/forth's (REQUIRE hydra.fs).
;   Subroutine threaded: a word's execution token is its code's address, and a definition is a run of `jsr xt`
; (literals and IF's test compiled inline; a few short words, the return stack's among them, copied in whole: F_INLINE).
; The data stack is the program's zero page, low bytes and high bytes apart (dlo, dhi), indexed by .X, which every
; word keeps as the stack pointer (DS_N: empty; it grows down); the return stack is the 6502's.  The dictionary is the
; task's RAM after the BSS, to DICT_END; the words in ROM have their headers beside their code, chained into the same
; list as the ones loaded or defined in RAM (FORTH's: a word list is a chain of headers).  A header: the link (2: the one before,
; 0 at the first), the name's length and flags (1: F_IMMEDIATE, F_HIDDEN, F_INLINE), the name (as typed: found
; ignoring case), then (F_INLINE) the code's length; the code, its xt, follows.  A header's address is its nt.
;   Input: stdin, a line at a time (the console's cooked lines, or a file's, through rc's <), or a file's (INCLUDED),
; or a string's (EVALUATE): the source before a nested one is kept on the source stack.  Output: fd 1, buffered.  A
; fileid is the system's fd; an ior is 0, or -512 less the system's error code (Gforth's way).  Errors are THROWs
; (Exception), caught in QUIT: the message (with the file and line, from a file), both stacks emptied, the files
; being included closed, and on with the next line.
;   The parts: fcore.inc (stacks, arithmetic, memory), fmath.inc (multiplication and division), ftext.inc (input,
; output, numbers, strings, parsing), fcomp.inc (the compiler: definitions, control flow, defining words), finterp.inc
; (the text interpreter, QUIT, CATCH and THROW, EVALUATE), ffile.inc (files: including them, loading a library),
; fscript.inc (Ctrl-C, scripts), fprog.inc (programs started and waited for: the libraries' SH, RUN and the shell's);
; fdefs.inc has the constants and HEADER, which the libraries use too.  Their words are
; in that order in the dictionary, then the libraries' as they're loaded.  A library calls the core's code by its
; label (obj/gen/forthcore.inc, the build's: the core's labels, as equates; a library is for the core it was built
; with, core_id); the core's code that it uses itself (PICK, AGAIN, CATCH, OPEN-FILE ...), and what compiled
; definitions call (?DO's, VALUE's, DEFER's ...), stays here, headerless, and the library's header jumps to it.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "fdefs.inc"

            HYX2_PROGRAM "forth", main

.zeropage
dlo:        .res        DS_N                                ; The data stack: low bytes ...
dhi:        .res        DS_N                                ;   and high bytes (.X: the top's index)
w:          .res        2                                   ; Scratch pointers ...
w2:         .res        2
w3:         .res        2
tmp:        .res        2                                   ;   and numbers
tmp2:       .res        2
tmp3:       .res        2
xsave:      .res        1                                   ; .X, kept over a system call or a tsx
cnt:        .res        1
here:       .res        2                                   ; The dictionary's next byte
p1:         .res        2                                   ; Pointers (strings, SEE)
p2:         .res        2
intr:       .res        1                                   ; $80: Ctrl-C came (the note handler's), for THROW -28

.bss
forth_wl:   .res        4                                   ; FORTH-WORDLIST: a word list is its last header (0: none),
                                                            ;   then the word list made before it (0: none)
wl_last:    .res        2                                   ; The word lists, newest first (a chain: FORTH's last)
current:    .res        2                                   ; The compilation word list
order_n:    .res        1                                   ; The search order: how many word lists ...
order:      .res        ORDER_MAX * 2                       ;   and they (the first searched first)
lastxt:     .res        2                                   ; The definition being made: its xt (RECURSE, DOES>) ...
lasthdr:    .res        2                                   ;   and its header (; shows it)
state:      .res        2                                   ; STATE: 0 interpreting, -1 compiling
base:       .res        2                                   ; BASE
src_addr:   .res        2                                   ; The input source (SRC_SIZE bytes, in this order: the
src_len:    .res        2                                   ;   source stack's records are copies): SOURCE ...
to_in:      .res        2                                   ;   >IN ...
src_id:     .res        2                                   ;   SOURCE-ID: 0 stdin, -1 a string, or a fileid ...
src_pos:    .res        4                                   ;   a file's: where the line SOURCE holds starts ...
src_cons:   .res        2                                   ;   the bytes it took (its end too) ...
src_line:   .res        2                                   ;   its number (stdin's too) ...
src_close:  .res        1                                   ;   <> 0: a file, closed at its end ...
src_fdep:   .res        1                                   ;   and the files being included (1 ...: this file's)
SRC_SIZE    = * - src_addr
ssp:        .res        1                                   ; The source stack's records (EVALUATE, INCLUDE-FILE)
sstack:     .res        SRC_SIZE * SRC_MAX
handler:    .res        2                                   ; CATCH's frame (the 6502's stack pointer at it), or 0
rsp0:       .res        1                                   ; The 6502's stack pointer as QUIT starts
interactive: .res       1                                   ; <> 0: stdin is the console (prompts)
leaves:     .res        2                                   ; The LEAVEs of the DO being compiled, a chain
hld:        .res        2                                   ; Pictured numeric output: the next char's place
throw_name: .res        2                                   ; The undefined word (its address and length)
throw_nlen: .res        1
abort_msg:  .res        2                                   ; ABORT"'s message (a counted string)
ctlfd:      .res        1                                   ; /dev/consctl's fd (KEY's raw mode), $FF: not open
sbuf_n:     .res        1                                   ; S" while interpreting: the buffer last used
ilen:       .res        2                                   ; stdin's bytes read ahead: how many ...
ipos:       .res        2                                   ;   and the next
olen:       .res        1                                   ; The output's bytes waiting
err_noted:  .res        1                                   ; <> 0: an error's place noted (THROW's), for QUIT: its
err_line:   .res        2                                   ;   line, and its file (0: stdin; 1 ...: lnames's)
err_fdep:   .res        1
throw_named: .res       1                                   ; <> 0: an OS error is throw_name's (INCLUDED's)
inc_named:  .res        1                                   ; <> 0: INCLUDED has named the file to come
rl_fd:      .res        1                                   ; READ-LINE's: the fd, the bytes read, the line's length
rl_n:       .res        2
rl_len:     .res        2
cond_lvl:   .res        1                                   ; [IF]'s skipping: how deep, and whether [ELSE] ends it
cond_else:  .res        1
incn_len:   .res        2                                   ; The files INCLUDED (REQUIRED's): their bytes in incn
raw:        .res        1                                   ; <> 0: the console in raw mode (KEY, KEY?)
key_pend:   .res        1                                   ; <> 0: KEY? has a key (key_char) for KEY
key_char:   .res        1
ibuf:       .res        IBUF_SIZE
tib:        .res        TIB_SIZE
obuf:       .res        OBUF_SIZE
holdbuf:    .res        HOLD_SIZE
hold_end:
pad:        .res        PAD_SIZE
wbuf:       .res        WBUF_SIZE
sbuf:       .res        SBUF_SIZE * 2
numacc:     .res        4                                   ; (Numbers: an accumulator, 32 bits ...
numtmp:     .res        4                                   ;   and another)
lbufs:      .res        LINE_BUF * INC_MAX                  ; The files being included: each one's line ...
lnames:     .res        LNAME_SIZE * INC_MAX                ;   and name (counted: INCLUDED's)
incn:       .res        INCN_SIZE                           ; The files INCLUDED: counted names
pathbuf:    .res        PATH_SIZE                           ; A file's name, zero-terminated (for the system)
statbuf:    .res        SR_SIZE                             ; A stat record
argp:       .res        2                                   ; forth's arguments (main's r0): a script's name first
script:     .res        1                                   ; <> 0: forth file.fs (the file run, then the end)
login:      .res        1                                   ; <> 0: forth -l (newns, then profile.fs)
lastc:      .res        1                                   ; The last character out (emit_a's)
out_hook:   .res        2                                   ; <> 0: what flush gives the output to (the shell's)
argbuf:     .res        ARGS_MAX                            ; A program's arguments (fprog.inc's) ...
prog_map:   .res        4                                   ;   and its fds (SPAWN_FDMAP's: 3, then fds 0-2)
libs_n:     .res        1                                   ; The libraries loaded, oldest first: how many records ...
libtab:     .res        LR_SIZE * LIB_MAX                   ;   and they (LR_*)
dict:                                                       ; The dictionary, from here

.segment "DATA"
; The note handler, in RAM (either bank may be at $A000 when a note comes): Ctrl-C (NOTE_INTERRUPT) noted in intr,
; for the next word, loop or wait to THROW -28, forth going on; another note, the default
notes:
            cmp         #NOTE_INTERRUPT
            bne         :+
            lda         #$80
            sta         intr
            clc
            rts
:
            sec
            rts

.code
; ****************************************************************************
; The start: the dictionary after the BSS, its end claimed (BREAK), decimal, stdin a console or not, Ctrl-C a
; THROW; the banner (not a script's), and startup.fs (its libraries); then the script, or QUIT
main:
            lda         r0                                  ; Its arguments: a script's name, and its own; or -l
            sta         argp
            lda         r0 + 1
            sta         argp + 1
            stz         script
            stz         login
            ora         r0
            beq         :+
            lda         (r0)
            beq         :+
            inc         script
            cmp         #'-'                                ; (-l: a login shell, no script)
            bne         :+
            ldy         #1
            lda         (r0),y
            cmp         #'l'
            bne         :+
            iny
            lda         (r0),y
            bne         :+
            stz         script
            inc         login
:
            LDR         r0, DICT_END
            jsr         BREAK
            LDR         r0, notes
            jsr         NOTIFY
            ldx         #DS_N
            lda         #<dict
            sta         here
            lda         #>dict
            sta         here + 1
            lda         #<forth_last                        ; FORTH: the ROM's words, and the order and
            sta         forth_wl                            ;   definitions in it alone
            lda         #>forth_last
            sta         forth_wl + 1
            stz         forth_wl + 2
            stz         forth_wl + 3
            lda         #<forth_wl
            sta         wl_last
            sta         current
            ldy         #>forth_wl
            sty         wl_last + 1
            sty         current + 1
            sta         order
            sty         order + 1
            lda         #1
            sta         order_n
            ldy         #SRC_SIZE - 1                       ; The source: stdin (no line yet), none nested
:
            lda         #0
            sta         src_addr,y
            dey
            bpl         :-
            stz         ssp
            stz         err_noted
            stz         throw_named
            stz         inc_named
            stz         incn_len
            stz         incn_len + 1
            stz         libs_n
            lda         #LF
            sta         lastc
            stz         out_hook
            stz         out_hook + 1
            stz         raw
            stz         key_pend
            stz         intr
            lda         #10
            sta         base
            stz         base + 1
            stz         handler
            stz         handler + 1
            stz         olen
            stz         ilen
            stz         ilen + 1
            stz         ipos
            stz         ipos + 1
            stz         sbuf_n
            lda         #$FF
            sta         ctlfd
            stz         interactive                         ; The console on stdin: prompts
            stx         xsave
            LDR         r0, pad                             ; (Its stat record, in pad: unused yet)
            lda         #0
            jsr         FSTAT
            ldx         xsave
            bcs         :+
            lda         pad + SR_DEV
            cmp         #'c'
            bne         :+
            inc         interactive
            lda         script                              ; (The banner: not a script's)
            bne         :+
            jsr         banner
:
            tsx
            stx         rsp0
            ldx         #DS_N
            lda         login                               ; (forth -l: its namespace first, as rc -l's: before it,
            beq         :+                                  ;   there's no /lib to load anything from)
            jsr         do_newns
:
            LDR         w, s_startup
            jsr         startup
            lda         login
            beq         :+
            LDR         w, s_profile
            jsr         startup
:
            lda         script
            beq         :+
            jmp         run_script
:
            jmp         quit

banner:
            LDR         w, s_banner
            jmp         type_z

s_banner:   .byte       "HyForth (Forth 2012), bye to end", LF, 0

; The file w (a counted name), if there is one, INCLUDED: /lib/forth/startup.fs, the libraries forth starts with (the
; ROM's, or a card's or the RAM disk's before it, through the /lib union); or, for forth -l, /lib/forth/profile.fs.
; An error in it: its message, and on
startup:
            lda         (w)
            pha
            clc
            lda         w
            adc         #1
            ldy         w + 1
            bcc         :+
            iny
:
            PUSHAY
            pla
            ldy         #0
            PUSHAY
            lda         #<included
            ldy         #>included
            PUSHAY
            jsr         catch
            lda         dlo,x
            ora         dhi,x
            beq         @done
            lda         dlo,x                               ; (None: nothing said)
            cmp         #<(-512 - E_NOENT)
            bne         @say
            lda         dhi,x
            cmp         #>(-512 - E_NOENT)
            bne         @say
            stz         throw_named
            stz         err_noted
@done:
            inx
            rts
@say:
            jmp         show_error

s_startup:  .byte       S_STARTUP_LEN, "/lib/forth/startup.fs"
S_STARTUP_LEN = * - s_startup - 1
s_profile:  .byte       S_PROFILE_LEN, "/lib/forth/profile.fs"
S_PROFILE_LEN = * - s_profile - 1

; The default namespace (nslib's newns, as init and rc build theirs): forth -l's, and NEWNS's (the Hydra's shell
; library: shell.fl).  Its buffers are the dictionary's last NS_BSS_SIZE bytes, so the dictionary must end below
; them: else THROW -8
do_newns:
            lda         here
            cmp         #<NS_BSS
            lda         here + 1
            sbc         #>NS_BSS
            bcc         :+
            lda         #<-8
            jmp         throw_a
:
            jsr         flush
            phx
            jsr         ns_default
            plx
            rts

; The core's id (tools/forthlib.js: a CRC of its image, patched in), which a library must have
core_id:    .word       0

.include "fcore.inc"
.include "fmath.inc"
.include "ftext.inc"
.include "fcomp.inc"
.include "finterp.inc"
.include "ffile.inc"
.include "fscript.inc"
.include "fprog.inc"

forth_last  = .ident(.sprintf("hdr_%d", hdr_n))             ; (The last ROM header: the word list's start)

; nslib (the SDK's: newns), with forth's scratch for its zero page (nothing of forth's runs in it) and the
; dictionary's top for its buffers (do_newns)
NS_ZP       = 1
ns_p        = w
ns_end      = w2
ns_w        = w3
ns_task     = tmp
ns_n        = tmp + 1
ns_fl       = tmp2
ns_fd       = tmp2 + 1
ns_d        = tmp3
ns_len      = tmp3 + 1
ns_line     = p1
NS_BSS      = DICT_END - NS_BSS_SIZE
.include "nslib.s"
