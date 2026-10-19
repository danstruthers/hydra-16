; ****************************************************************************
; cmdmatch.s - Dingus Quest Zero's command matcher, for the 65C02: does a line parse by the command grammar?  The
; host's reference is sim/tools/commandcompile.js; the tables are sim/tools/commandtables.js's (the grammar, DQ0G
; version 7; its index, DQ0I version 2; the default aliases, DQ0A version 1); the test data, commandmatch-gen.js's.
;
; Its calls (the parser's task, dq0c.s, makes them; C's cmdmatch.h, sim/tools/cmdmatch/cmdmatch_c.s, wraps them):
;   cm_init     the tables: cm_g the grammar, cm_i its index, cm_a the default aliases (each where it stays).  C = 0,
;               or C = 1: one is not a dq0 table, they don't belong together, or one is too large for the matcher
;   cm_world    the world's words, when they change: cm_g the words the world names things by, cm_i the names
;               given, cm_a the deities' names (space-separated lists, each to stay as it is till the next cm_world)
;   cm_match    a line (.A/.X, zero-ended): .A 1 if it parses, 0 if not, $FF (and .X $FF) if it is too long or
;               nests too deep
;
; The matcher returns, for a node at a position, the positions where it can end, as a 16-bit mask (bit p: the match
; ends before token p).  It walks the grammar as it stands in its table; nothing is built from it but each rule's
; address.  What keeps it quick:
; - The words are found in the index's dictionary: a Pearson hash of the word picks its bucket, and the bucket's words
;   are compared.  Each entry has its lexicon flags (LX_*: the words the open names look for), the world's marks
;   (W_*: cm_world, when the world changes), and heads the chain of the aliases that begin with its word: the
;   aliases are a trie of words, the first by the hash, the rest compared.
; - A rule is entered only if the token can begin it: its first words as a 64-bit Bloom filter, and its first kinds.
;   An alternation of many options (the phrases) tries only the options the token begins: its first words, sorted, by
;   a binary search; and its options that begin with an open name the token fits, or can match nothing.
; - A rule's ends are kept in a cache (128 entries, a line's), not a memo of every rule at every position: the grammar
;   has no left recursion (commandtables.js says so as it builds the index), so nothing needs a "being matched" mark.
; - A word, an open name and quoted text are checked where they stand in a sequence or alternation, without a call.
; The return addresses go on the 6502's stack (two bytes a level, the grammar's nesting at most); a composite node's
; working values on a stack of their own (fstk).  A line that would nest deeper than either allows gives -1.
;
; Its size: 4.6K of code; 4K of RAM (the tokens, the cache, the working values' stack, the dictionary's marks and
; alias chains, the aliases, the rules' addresses) and 42 bytes of zero page.  The tables are read where they are: the
; grammar may be in a RAM bank at $8000 while it matches (sim/tools/cmdmatch/cmdhy.c), the index and the aliases not.
; Speed (sim/tools/commandmatch-test.js): about 34,000 cycles a command on the corpus; 37,000 under HydraOS.
; ****************************************************************************

            .setcpu     "65C02"
            .macpack    longbranch
            .export     cm_init, cm_world, cm_match, cm_g, cm_i, cm_a

; Node tags in the grammar table
T_SEQ       = 1
T_ALT       = 2
T_OPT       = 3
T_REP       = 4
T_NT        = 5
T_TERM      = 6
T_QUOTE     = 7

; The open-name kinds, a bit each in a token's mask (tokmlo: kinds 0-7, tokmhi: 8-10)
K_OBJ       = $01                   ; (lo)
K_ADJ       = $02
K_NUMBER    = $04
K_ORDINAL   = $08
K_TASK      = $10
K_SPELL     = $20
K_TEXT      = $40
K_NAMEWORD  = $80
K_DEITY     = $01                   ; (hi)
K_PRETENSE  = $02
K_QUOTED    = $04

; The lexicon's flags (commandtables.js's LX) and the world's marks
LX_ADJ      = 1
LX_TASK     = 2
LX_ORDINAL  = 4
LX_NUMBER   = 8
LX_NOTSPELL = 16
LX_NOTOBJECT = 32
W_WORLD     = 1                     ; The world names a thing by the word
W_NAME      = 2                     ; A name given to a thing
W_DEITY     = 4                     ; A deity's name

MAXTOK      = 14                    ; Positions 0-14 fit a 16-bit mask
MAXRULES    = 240                   ; (A rule's number, and a name's, is a byte)
MAXENT      = 600                   ; The dictionary's entries (the index's, 428 now, then the world's own words)
MAXDYN      = 16                    ; The world's own words: deities' names that are no word of the dictionary's
MAXALIAS    = 96
MAXINDEX    = 24
ALTMIN      = 8                     ; commandtables.js's ALT_INDEX_MIN: an alternation this large may be indexed
CACHE       = 128                   ; The rules' ends kept, a line's (a power of two)
MAXREP      = 12
FSTKMAX     = 240                   ; The working values' stack: a page, and a little room
HWMIN       = $30                   ; The 6502's stack: at least this much left for interrupts
TBUFMAX     = 180                   ; A line's letters kept

            .zeropage
np:         .res        2           ; ends: the node
pos:        .res        1           ; ends: the position
res:        .res        2           ; ends: the positions it can end at
rid:        .res        1           ; A rule's number
cp:         .res        2           ; The working values (a composite node's, kept on fstk by each that uses them):
wn:         .res        1           ;   the child, the count left,
wm:         .res        2           ;   the positions to start from, the positions reached,
wo:         .res        2
wq:         .res        1           ;   a position kept, the position scanned, what a repeat has seen
wp:         .res        1
ws:         .res        2
lfp:        .res        2           ; leaf: its node
tp:         .res        2           ; Pointers and bytes for the moment
tq:         .res        2
tr:         .res        2
tl:         .res        1
th:         .res        1
t1:         .res        1
t2:         .res        1
t3:         .res        1
pperm:      .res        2           ; The hash's permutation
pkind:      .res        2           ; The open names' kinds
pklo:       .res        2           ; The rules' first kinds, and (pkhi's top bit) whether they can match nothing
pkhi:       .res        2
lnp:        .res        2           ; tokenize's line; cm_world's list; cm_init's aliases
te:         .res        2           ; lookup: the bucket's end

            .bss
gbase:      .res        2           ; The grammar table
nrules:     .res        1
nnames:     .res        1
nwords:     .res        2
rplo:       .res        MAXRULES    ; Each rule's body
rphi:       .res        MAXRULES
nent:       .res        2           ; The index's dictionary: its entries,
rline:      .res        1           ;   the rules treated by name,
rseq:       .res        1
rquoted:    .res        1
dstart:     .res        2           ;   where each hash's words start, the words, their flags
drec:       .res        2
dflags:     .res        2
brlo:       .res        8           ; The rules' first words: the Bloom filter's rows
brhi:       .res        8
nindex:     .res        1           ; The indexed alternations: each one's node, options, first words (and their
ixnlo:      .res        MAXINDEX    ;   options), count, and its open options and their count
ixnhi:      .res        MAXINDEX
ixalo:      .res        MAXINDEX
ixahi:      .res        MAXINDEX
ixwlo:      .res        MAXINDEX
ixwhi:      .res        MAXINDEX
ixolo:      .res        MAXINDEX
ixohi:      .res        MAXINDEX
ixclo:      .res        MAXINDEX
ixchi:      .res        MAXINDEX
ixplo:      .res        MAXINDEX
ixphi:      .res        MAXINDEX
ixpn:       .res        MAXINDEX
thenlo:     .res        1           ; The word "then"
thenhi:     .res        1
linenode:   .res        3           ; The rule "line", as a node
lmark:      .res        MAXENT      ; Each entry's marks (W_*)
ahead:      .res        MAXENT      ; Each entry's first alias, + 1 (0: none)
ndyn:       .res        1           ; The world's own words: their letters, lengths and hashes
dynlo:      .res        MAXDYN
dynhi:      .res        MAXDYN
dynlen:     .res        MAXDYN
dynhash:    .res        MAXDYN
nalias:     .res        1           ; The aliases: the next for the same word, stands anywhere, the verb's entry,
anext:      .res        MAXALIAS    ;   and the rest of the key (after its first word)
aany:       .res        MAXALIAS
avlo:       .res        MAXALIAS
avhi:       .res        MAXALIAS
arlo:       .res        MAXALIAS
arhi:       .res        MAXALIAS
arlen:      .res        MAXALIAS
tbuf:       .res        256         ; The line's words, in lower case
ntok:       .res        1           ; The tokens: count, then a line, too long a line
hasthen:    .res        1
toolong:    .res        1
tokoff:     .res        MAXTOK + 1  ; Each token's letters in tbuf, its length, quoted text or not, its grammar word
toklen:     .res        MAXTOK + 1  ;   ($FFFF: none), its dictionary entry ($FFFF: none), the kinds it fits, and
tokq:       .res        MAXTOK + 1  ;   its Bloom byte and bit (0: no word)
tokwl:      .res        MAXTOK + 1
tokwh:      .res        MAXTOK + 1
tokel:      .res        MAXTOK + 1
tokeh:      .res        MAXTOK + 1
tokmlo:     .res        MAXTOK + 1
tokmhi:     .res        MAXTOK + 1
tokbk:      .res        MAXTOK + 1
tokbb:      .res        MAXTOK + 1
ckr:        .res        CACHE       ; The cache: rule, position ($FF: empty), ends
ckp:        .res        CACHE
cml:        .res        CACHE
cmh:        .res        CACHE
fsp:        .res        1           ; The working values' stack
fstk:       .res        256
savedsp:    .res        1           ; The 6502's stack as cm_match found it
gen:        .res        1           ; The line's generation in the cache (its top four bits), and its position with it
posg:       .res        1
li:         .res        1           ; tokenize: where in the line, where in tbuf, a word's start
nb:         .res        1
tstart:     .res        1
cm_a:       .res        2           ; cm_init's tables, cm_world's lists (their callers')
cm_i:       .res        2
cm_g:       .res        2
lflag:      .res        1
bestn:      .res        1           ; aliases: the longest key matched, its alias
besta:      .res        1
rc:         .res        1           ; restmatch: the tokens taken, a word's start, end and length, the token
rs:         .res        1
re:         .res        1
rw:         .res        1
rj:         .res        1
blo:        .res        2           ; alt_ix: the binary search, and the word looked for
bhi:        .res        2
bmid:       .res        2
bw:         .res        2
klen:       .res        1           ; rdaliases: a key's length, the alias
ai:         .res        1

            .rodata
bitlo:      .byte       $01, $02, $04, $08, $10, $20, $40, $80, 0, 0, 0, 0, 0, 0, 0, 0
bithi:      .byte       0, 0, 0, 0, 0, 0, 0, 0, $01, $02, $04, $08, $10, $20, $40, $80
bitb:       .byte       $01, $02, $04, $08, $10, $20, $40, $80
; The kinds 0-7 a word's lexicon flags give (less LX_NOTOBJECT's): text, a spell's word unless LX_NOTSPELL, and the rest
kindtab:
            .repeat     32, F
            .byte       K_TEXT | ((F & LX_NOTSPELL) ^ LX_NOTSPELL) * 2 | (F & LX_ADJ) * 2 | (F & LX_TASK) * 8 | (F & LX_ORDINAL) * 2 | (F & LX_NUMBER) / 2
            .endrepeat
posmix:     .byte       0, 37, 74, 111, 20, 57, 94, 3, 40, 77, 114, 23, 60, 97, 6, 43   ; A position's part of the cache's hash
thenword:   .byte       "then"

            .code

; ---- The dictionary ------------------------------------------------------------------------------------------------

; hash: Pearson's hash of the tl letters at tp (commandtables.js's wordHash).  OUT: .A and th
.proc hash
            lda         #0
            ldy         #0
loop:       cpy         tl
            beq         done
            eor         (tp),y
            sty         t1
            tay
            lda         (pperm),y
            ldy         t1
            iny
            bra         loop
done:       eor         tl
            tay
            lda         (pperm),y
            sta         th
            rts
.endproc

; lookup: the dictionary's entry for the tl letters at tp: the index's words with its hash, then the world's own.
; OUT: C clear and .A/.X the entry (lo/hi); C set and $FFFF if none.  th: the hash.
.proc lookup
            jsr         hash
            asl         a               ; tq: the hash's start, two bytes in dstart
            sta         tq
            lda         #0
            rol         a
            sta         tq+1
            clc
            lda         tq
            adc         dstart
            sta         tq
            lda         tq+1
            adc         dstart+1
            sta         tq+1
            ldy         #0              ; tr: its first word; te: past its last
            clc
            lda         (tq),y
            adc         drec
            sta         tr
            iny
            lda         (tq),y
            adc         drec+1
            sta         tr+1
            iny
            clc
            lda         (tq),y
            adc         drec
            sta         te
            iny
            lda         (tq),y
            adc         drec+1
            sta         te+1
            sec                         ; tq: the letters less one, to compare with a word's (after its length)
            lda         tp
            sbc         #1
            sta         tq
            lda         tp+1
            sbc         #0
            sta         tq+1
next:       lda         tr
            cmp         te
            bne         word
            lda         tr+1
            cmp         te+1
            beq         dyn
word:       lda         (tr)            ; a word: its length, its letters, its entry
            cmp         tl
            bne         skip
            tay
            beq         found
same:       lda         (tr),y
            cmp         (tq),y
            bne         skip
            dey
            bne         same
found:      ldy         tl
            iny
            lda         (tr),y
            pha
            iny
            lda         (tr),y
            tax
            pla
            clc
            rts
skip:       lda         (tr)
            sec                         ; (past its length, its letters and its entry)
            adc         #2
            adc         tr
            sta         tr
            bcc         next
            inc         tr+1
            bra         next
dyn:        ldx         #0              ; the world's own words
dloop:      cpx         ndyn
            bcs         none
            lda         dynhash,x
            cmp         th
            bne         dnext
            lda         dynlen,x
            cmp         tl
            bne         dnext
            lda         dynlo,x
            sta         tr
            lda         dynhi,x
            sta         tr+1
            ldy         tl
            beq         dfound
dsame:      dey
            lda         (tr),y
            cmp         (tp),y
            bne         dnext
            cpy         #0
            bne         dsame
dfound:     txa                         ; its entry: after the index's
            clc
            adc         nent
            pha
            lda         nent+1
            adc         #0
            tax
            pla
            clc
            rts
dnext:      inx
            bra         dloop
none:       lda         #$FF
            tax
            sec
            rts
.endproc

; seq_head: a sequence's or alternation's first child into cp.  OUT: .A its count
.proc seq_head
            ldy         #1
            lda         (np),y
            bmi         big
            ldy         #2
            bra         got
big:        ldy         #3
got:        lda         (np),y
            pha
            iny
            tya
            clc
            adc         np
            sta         cp
            lda         np+1
            adc         #0
            sta         cp+1
            pla
            rts
.endproc

; opt_head: an option's or a repeat's child into np
.proc opt_head
            ldy         #1
            lda         (np),y
            bmi         big
            lda         #2
            bra         add
big:        lda         #3
add:        clc
            adc         np
            sta         np
            bcc         done
            inc         np+1
done:       rts
.endproc

; skip: cp past its node: a composite by its size, a leaf by its length
.proc skip
            lda         (cp)
            cmp         #T_NT
            beq         three
            cmp         #T_TERM
            beq         term
            cmp         #T_QUOTE
            beq         one
            ldy         #1
            lda         (cp),y
            bmi         big
            bra         add
big:        and         #$7F
            sta         t1
            iny
            lda         (cp),y
            clc
            adc         cp
            sta         cp
            lda         t1
            adc         cp+1
            sta         cp+1
            rts
term:       ldy         #1              ; 2, and 2 a word
            lda         (cp),y
            stz         t1
            asl         a
            rol         t1
            clc
            adc         #2
            bcc         :+
            inc         t1
:           clc
            adc         cp
            sta         cp
            lda         t1
            adc         cp+1
            sta         cp+1
            rts
three:      lda         #3
            bra         add
one:        lda         #1
add:        clc
            adc         cp
            sta         cp
            bcc         done
            inc         cp+1
done:       rts
.endproc

; packsize: a packed word's bytes, with its length's: 1 + (5 * .A + 7) / 8
.proc packsize
            sta         t3
            sta         t1
            stz         t2
            asl         t1
            rol         t2
            asl         t1
            rol         t2
            clc
            lda         t1
            adc         t3
            sta         t1
            lda         t2
            adc         #0
            sta         t2
            clc
            lda         t1
            adc         #7
            sta         t1
            lda         t2
            adc         #0
            lsr         a
            ror         t1
            lsr         a
            ror         t1
            lsr         a
            ror         t1
            lda         t1
            ina
            rts
.endproc

; zeroent: MAXENT bytes from tp zeroed (lmark, ahead)
.proc zeroent
            lda         #0
            ldx         #>MAXENT
            ldy         #0
page:       cpx         #0
            beq         rest
:           sta         (tp),y
            iny
            bne         :-
            inc         tp+1
            dex
            bra         page
rest:       ldy         #<MAXENT
            beq         done
:           dey
            sta         (tp),y
            bne         :-
done:       rts
.endproc

; ---- A line's tokens -----------------------------------------------------------------------------------------------

; alnum: is .A a letter or a digit?  C set if so (.A kept)
.proc alnum
            cmp         #'0'
            bcc         no
            cmp         #'9' + 1
            bcc         yes
            cmp         #'A'
            bcc         no
            cmp         #'Z' + 1
            bcc         yes
            cmp         #'a'
            bcc         no
            cmp         #'z' + 1
            bcc         yes
no:         clc
            rts
yes:        sec
            rts
.endproc

; tokenize: the line at lnp into tokens: words (lower case, with ' and - after the first letter), ! and =, and quoted
; text (a w or y directly before the quote is part of it); anything else between them.  Each word is looked up as it
; is read.  Then the aliases, then each token's kinds and Bloom bit.
.proc tokenize
            stz         ntok
            stz         toolong
            stz         hasthen
            stz         nb
            stz         li
            ldy         #0              ; (a line of 255 letters at most)
len:        lda         (lnp),y
            beq         next
            iny
            bne         len
            jmp         long
next:       ldy         li
            lda         (lnp),y
            bne         have
            jmp         done
have:       cmp         #' '
            bne         notsp
skipc:      inc         li
            bra         next
notsp:      cmp         #'"'
            beq         quote
            cmp         #'w'
            beq         wy
            cmp         #'y'
            bne         notwy
wy:         iny
            lda         (lnp),y
            cmp         #'"'
            bne         wyword
            inc         li              ; (at the quote)
            bra         quote
wyword:     ldy         li
            lda         (lnp),y
notwy:      jsr         alnum
            jcs         word
            cmp         #'!'
            beq         punct
            cmp         #'='
            beq         punct
            bra         skipc
quote:      ldx         ntok
            cpx         #MAXTOK
            bcs         long
            inc         li
            lda         #1
            sta         tokq,x
            lda         #$FF
            sta         tokwl,x
            sta         tokwh,x
            sta         tokel,x
            sta         tokeh,x
            lda         nb
            sta         tokoff,x
ql:         ldy         li
            lda         (lnp),y
            beq         qend
            cmp         #'"'
            beq         qclose
            ldy         nb
            cpy         #TBUFMAX
            bcs         qend
            sta         tbuf,y
            inc         nb
            inc         li
            bra         ql
qclose:     inc         li
qend:       ldx         ntok
            lda         nb
            sec
            sbc         tokoff,x
            sta         toklen,x
            inc         nb
            inc         ntok
            jmp         next
long:       lda         #1
            sta         toolong
            rts
punct:      ldx         ntok
            cpx         #MAXTOK
            bcs         long
            ldy         nb
            sty         tstart
            sta         tbuf,y
            inc         nb
            inc         li
            bra         tok
word:       ldx         ntok
            cpx         #MAXTOK
            bcs         long
            lda         nb
            sta         tstart
wl:         ldy         li
            lda         (lnp),y
            jsr         alnum
            bcs         wch
            cmp         #$27            ; '
            beq         wch
            cmp         #'-'
            bne         tok
wch:        cmp         #'A'
            bcc         wlow
            cmp         #'Z' + 1
            bcs         wlow
            ora         #$20
wlow:       ldy         nb
            cpy         #TBUFMAX
            bcs         wfull
            sta         tbuf,y
            inc         nb
wfull:      inc         li
            bra         wl
tok:        ldx         ntok            ; the token: tbuf from tstart to nb
            stz         tokq,x
            lda         tstart
            sta         tokoff,x
            lda         nb
            sec
            sbc         tstart
            sta         toklen,x
            sta         tl
            clc
            lda         #<tbuf
            adc         tstart
            sta         tp
            lda         #>tbuf
            adc         #0
            sta         tp+1
            jsr         lookup
            ldy         ntok
            sta         tokel,y
            txa
            sta         tokeh,y
            jsr         wordof
            lda         tokwl,y
            cmp         thenlo
            bne         :+
            lda         tokwh,y
            cmp         thenhi
            bne         :+
            lda         #1
            sta         hasthen
:           inc         nb
            inc         ntok
            jmp         next
done:       jsr         aliases
            jmp         kinds
.endproc

; wordof: token .Y's grammar word: its entry if that is one of the grammar's words, else $FFFF (.Y kept)
.proc wordof
            lda         tokeh,y
            cmp         nwords+1
            bcc         yes
            bne         no
            lda         tokel,y
            cmp         nwords
            bcs         no
yes:        lda         tokel,y
            sta         tokwl,y
            lda         tokeh,y
            sta         tokwh,y
            rts
no:         lda         #$FF
            sta         tokwl,y
            sta         tokwh,y
            rts
.endproc

; aliases: a command's verb at the start of the line or after "then", a direction word anywhere; the longest key wins.
; The token's entry heads its word's aliases; the rest of a key (its words after the first) is compared with the
; tokens after it.  The verb's entry takes the token's place, and the key's other tokens go.
.proc aliases
            stz         t3              ; t3: the token
tloop:      ldx         t3
            cpx         ntok
            bcc         go
            rts
go:         lda         tokq,x
            jne         tnext
            lda         tokeh,x
            cmp         #$FF
            jeq         tnext
            clc                         ; its word's first alias
            lda         tokel,x
            adc         #<ahead
            sta         tq
            lda         tokeh,x
            adc         #>ahead
            sta         tq+1
            lda         (tq)
            beq         tnext
            sta         t1              ; t1: the alias, + 1
            stz         t2              ; t2: 1 at a command's start
            cpx         #0
            beq         seg
            lda         tokwl-1,x
            cmp         thenlo
            bne         noseg
            lda         tokwh-1,x
            cmp         thenhi
            bne         noseg
seg:        inc         t2
noseg:      stz         bestn
chain:      ldy         t1
            dey
            lda         t2
            bne         ok
            lda         aany,y
            beq         anext1
ok:         lda         arlen,y
            beq         one
            phy
            jsr         restmatch
            ply
            bra         got
one:        lda         #1
got:        cmp         bestn
            bcc         anext1
            beq         anext1
            sta         bestn
            sty         besta
anext1:     ldy         t1
            dey
            lda         anext,y
            sta         t1
            bne         chain
            lda         bestn
            beq         tnext
            ldy         besta           ; the verb in the token's place
            ldx         t3
            lda         avlo,y
            sta         tokel,x
            lda         avhi,y
            sta         tokeh,x
            txa
            tay
            jsr         wordof
            lda         bestn
            dea
            beq         tnext
            ldx         t3
            jsr         drop
tnext:      inc         t3
            jmp         tloop
.endproc

; restmatch: alias .Y's rest of key against the tokens after token t3.  OUT: .A the tokens it takes with the first
; (0: no match)
.proc restmatch
            lda         arlo,y
            sta         tr
            lda         arhi,y
            sta         tr+1
            lda         arlen,y
            sta         tl
            lda         #1
            sta         rc
            lda         t3
            sta         rj
            ldy         #0
word:       inc         rj
            sty         rs
scan:       cpy         tl
            beq         wend
            lda         (tr),y
            cmp         #' '
            beq         wend
            iny
            bra         scan
wend:       sty         re
            ldx         rj
            cpx         ntok
            bcs         no
            lda         tokq,x
            bne         no
            tya
            sec
            sbc         rs
            cmp         toklen,x
            bne         no
            sta         rw
            clc                         ; tp: the key's word; tq: the token's letters
            lda         tr
            adc         rs
            sta         tp
            lda         tr+1
            adc         #0
            sta         tp+1
            clc
            lda         #<tbuf
            adc         tokoff,x
            sta         tq
            lda         #>tbuf
            adc         #0
            sta         tq+1
            ldy         rw
            beq         same
cmpl:       dey
            lda         (tp),y
            cmp         (tq),y
            bne         no
            cpy         #0
            bne         cmpl
same:       inc         rc
            ldy         re
            cpy         tl
            beq         yes
            iny
            bra         word
yes:        lda         rc
            rts
no:         lda         #0
            rts
.endproc

; drop: the .A tokens after token .X go
.proc drop
            sta         t1
            inx
loop:       txa
            clc
            adc         t1
            cmp         ntok
            bcs         done
            tay
            lda         tokoff,y
            sta         tokoff,x
            lda         toklen,y
            sta         toklen,x
            lda         tokq,y
            sta         tokq,x
            lda         tokwl,y
            sta         tokwl,x
            lda         tokwh,y
            sta         tokwh,x
            lda         tokel,y
            sta         tokel,x
            lda         tokeh,y
            sta         tokeh,x
            inx
            bra         loop
done:       lda         ntok
            sec
            sbc         t1
            sta         ntok
            rts
.endproc

; kinds: what open names each token fits (the host's openOK), from its lexicon flags and the world's marks; and its
; Bloom byte and bit (its grammar word's bits 3-5 and 0-2)
.proc kinds
            ldx         #0
loop:       cpx         ntok
            bcc         one
            rts
one:        lda         tokq,x
            beq         word
            stz         tokmlo,x
            lda         #K_QUOTED
            sta         tokmhi,x
            jmp         bloom
word:       stz         t1              ; t1: the flags; t2: the marks
            stz         t2
            lda         tokeh,x
            cmp         #$FF
            beq         kind
            clc
            lda         tokel,x
            adc         #<lmark
            sta         tq
            lda         tokeh,x
            adc         #>lmark
            sta         tq+1
            lda         (tq)
            sta         t2
            lda         tokeh,x         ; the flags: the index's entries have them
            cmp         nent+1
            bcc         flags
            bne         kind
            lda         tokel,x
            cmp         nent
            bcs         kind
flags:      clc
            lda         tokel,x
            adc         dflags
            sta         tq
            lda         tokeh,x
            adc         dflags+1
            sta         tq+1
            lda         (tq)
            sta         t1
kind:       lda         t1              ; t3: the kinds 0-7 (from its flags); th: 8-10
            and         #31
            tay
            lda         kindtab,y
            sta         t3
            stz         th
            lda         t1
            and         #LX_NUMBER
            bne         num
            lda         toklen,x        ; or all digits
            beq         notnum
            sta         tl
            ldy         tokoff,x
digit:      lda         tbuf,y
            cmp         #'0'
            bcc         notnum
            cmp         #'9' + 1
            bcs         notnum
            iny
            dec         tl
            bne         digit
num:        lda         #K_NUMBER
            tsb         t3
notnum:     lda         tokwh,x
            cmp         #$FF
            bne         reserved
            lda         #K_PRETENSE     ; not a grammar word: a name, a word to pretend
            tsb         th
            lda         #K_NAMEWORD
            tsb         t3
            bra         obj
reserved:   lda         t2              ; a grammar word the world names a thing by
            and         #W_WORLD
            beq         named
            lda         #K_NAMEWORD
            tsb         t3
obj:        lda         t1
            and         #LX_NOTOBJECT
            bne         deity
            lda         #K_OBJ
            tsb         t3
            bra         deity
named:      lda         t2              ; or a name given to a thing
            and         #W_NAME
            beq         deity
            lda         t1
            and         #LX_NOTOBJECT
            bne         deity
            lda         #K_OBJ
            tsb         t3
deity:      lda         t2
            and         #W_DEITY
            beq         store
            lda         #K_DEITY
            tsb         th
store:      lda         t3
            sta         tokmlo,x
            lda         th
            sta         tokmhi,x
bloom:      lda         tokwh,x
            cmp         #$FF
            bne         hasw
            stz         tokbk,x
            stz         tokbb,x
            bra         nexttok
hasw:       lda         tokwl,x
            lsr         a
            lsr         a
            lsr         a
            and         #7
            sta         tokbk,x
            lda         tokwl,x
            and         #7
            tay
            lda         bitb,y
            sta         tokbb,x
nexttok:    inx
            jmp         loop
.endproc

; ---- The walk ------------------------------------------------------------------------------------------------------

; A working value kept on fstk (with .X fsp's copy), and taken back (in the reverse order)
.macro KEEP v
            lda         v
            sta         fstk,x
            inx
.endmacro
.macro TAKE v
            dex
            lda         fstk,x
            sta         v
.endmacro

; abort: the line nests too deep for the stacks: back to cm_match's caller with -1
.proc abort
            ldx         savedsp
            txs
            lda         #$FF
            tax
            rts
.endproc

; ends: the positions node np can end at from position .A, into res.  Each composite keeps the working values it uses
; and gives them back; a rule's reference keeps its rule and position.
.proc ends
            sta         pos
            tsx
            cpx         #HWMIN
            bcc         deep
            lda         (np)
            cmp         #8
            bcs         none
            asl         a
            tax
            jmp         (table,x)
deep:       jmp         abort
none:       stz         res
            stz         res+1
            rts
table:      .word       none, e_seq, e_alt, e_opt, e_rep, e_nt, e_leaf, e_leaf
.endproc

; setend: res: position .A's bit, or nothing if .A is $FF
.proc setend
            cmp         #$FF
            beq         zero
            tay
            lda         bitlo,y
            sta         res
            lda         bithi,y
            sta         res+1
            rts
zero:       stz         res
            stz         res+1
            rts
.endproc

; leaf: a word (or words), quoted text or an open name at lfp, from position .X.  OUT: C clear: not a leaf (a rule);
; C set: .A the position it ends at, $FF if it doesn't match
.proc leaf
            stx         t2
            lda         (lfp)
            cmp         #T_TERM
            beq         term
            cmp         #T_NT
            beq         nt
            cmp         #T_QUOTE
            beq         quote
            clc
            rts
term:       ldy         #1
            lda         (lfp),y
            sta         t1
            clc
            adc         t2
            cmp         ntok
            beq         :+
            bcs         no
:           ldy         #2
words:      lda         (lfp),y
            cmp         tokwl,x
            bne         no
            iny
            lda         (lfp),y
            cmp         tokwh,x
            bne         no
            iny
            inx
            dec         t1
            bne         words
            txa
            sec
            rts
quote:      cpx         ntok
            bcs         no
            lda         tokq,x
            beq         no
one:        inx
            txa
            sec
            rts
nt:         ldy         #1
            lda         (lfp),y
            cmp         rquoted
            beq         quote
            cmp         nrules
            bcc         rule
            cpx         ntok
            bcs         no
            sec                         ; the open name's number: its kind
            sbc         nrules
            tay
            lda         (pkind),y
            cmp         #8
            bcs         khi
            tay
            lda         bitb,y
            and         tokmlo,x
            bne         one
            bra         no
khi:        sbc         #8
            tay
            lda         bitb,y
            and         tokmhi,x
            bne         one
no:         lda         #$FF
            sec
            rts
rule:       clc
            rts
.endproc

; canbegin: can rule .A begin at position .X (a sequence needs a then)?  Its first words (the Bloom filter) and kinds,
; or it can match nothing.  C set if it can.
.proc canbegin
            tay
            cmp         rseq
            bne         gate
            lda         hasthen
            beq         no
gate:       lda         (pkhi),y
            bmi         yes             ; it can match nothing
            cpx         ntok
            bcs         no
            sty         t1
            lda         tokbb,x         ; the token's word among its first words?
            beq         kinds
            lda         tokbk,x
            tay
            lda         brlo,y
            sta         tq
            lda         brhi,y
            sta         tq+1
            ldy         t1
            lda         (tq),y
            and         tokbb,x
            bne         yes
kinds:      ldy         t1              ; or a kind of the token's among its first kinds?
            lda         (pklo),y
            and         tokmlo,x
            bne         yes
            lda         (pkhi),y
            and         tokmhi,x
            bne         yes
no:         clc
            rts
yes:        sec
            rts
.endproc

; A word or quoted text on its own
.proc e_leaf
            lda         np
            sta         lfp
            lda         np+1
            sta         lfp+1
            ldx         pos
            jsr         leaf
            jmp         setend
.endproc

; A name: an open name's leaf, or a rule: from the cache, or entered if the token can begin it
.proc e_nt
            lda         np
            sta         lfp
            lda         np+1
            sta         lfp+1
            ldx         pos
            jsr         leaf
            bcc         rule
            jmp         setend
rule:       ldy         #1
            lda         (np),y
            sta         rid
            ldx         pos             ; the cache
            txa
            ora         gen
            sta         posg
            lda         posmix,x
            clc
            adc         rid
            and         #CACHE - 1
            tax
            lda         ckp,x
            cmp         posg
            bne         miss
            lda         ckr,x
            cmp         rid
            bne         miss
            lda         cml,x
            sta         res
            lda         cmh,x
            sta         res+1
            rts
miss:       lda         rid
            ldx         pos
            jsr         canbegin
            bcs         enter
            stz         res
            stz         res+1
            rts
enter:      ldx         fsp
            KEEP        rid
            KEEP        pos
            stx         fsp
            cpx         #FSTKMAX
            bcs         over
            ldy         rid
            lda         rplo,y
            sta         np
            lda         rphi,y
            sta         np+1
            lda         pos
            jsr         ends
            ldx         fsp
            TAKE        pos
            TAKE        rid
            stx         fsp
            ldx         pos             ; into the cache
            lda         posmix,x
            clc
            adc         rid
            and         #CACHE - 1
            tax
            lda         rid
            sta         ckr,x
            lda         pos
            ora         gen
            sta         ckp,x
            lda         res
            sta         cml,x
            lda         res+1
            sta         cmh,x
            rts
over:       jmp         abort
.endproc

; A child (cp) from position wp: a leaf where it stands, anything else by ends; its ends into wo
.proc child
            lda         cp
            sta         lfp
            lda         cp+1
            sta         lfp+1
            ldx         wp
            jsr         leaf
            bcc         call
            cmp         #$FF
            beq         done
            tay
            lda         bitlo,y
            tsb         wo
            lda         bithi,y
            tsb         wo+1
done:       rts
call:       lda         (cp)            ; a sequence: can its first item begin here?
            cmp         #T_SEQ
            bne         really
            ldy         #1
            lda         (cp),y
            bmi         big
            ldy         #2
            bra         :+
big:        ldy         #3
:           lda         (cp),y          ; (its count)
            beq         really
            iny
            tya
            clc
            adc         cp
            sta         lfp
            lda         cp+1
            adc         #0
            sta         lfp+1
            ldx         wp
            jsr         leaf
            bcc         first
            cmp         #$FF
            beq         done
            bra         really
first:      lda         (lfp)           ; a rule first: can the token begin it?  (Anything else: the call)
            cmp         #T_NT
            bne         really
            ldy         #1
            lda         (lfp),y
            ldx         wp
            jsr         canbegin
            bcc         done
really:     lda         cp
            sta         np
            lda         cp+1
            sta         np+1
            lda         wp
            jsr         ends
            lda         res
            tsb         wo
            lda         res+1
            tsb         wo+1
            rts
.endproc

; scan: each position in wm (from wq up) through child, wm emptied, the ends into wo
.proc scan
            stz         wo
            stz         wo+1
            lda         wq
            sta         wp
loop:       lda         wm
            ora         wm+1
            beq         done
            ldx         wp
            lda         bitlo,x
            and         wm
            bne         hit
            lda         bithi,x
            and         wm+1
            beq         next
hit:        lda         bitlo,x
            trb         wm
            lda         bithi,x
            trb         wm+1
            jsr         child
next:       inc         wp
            bra         loop
done:       rts
.endproc

; A sequence: the positions so far, through each item in turn
.proc e_seq
            ldx         fsp
            KEEP        cp
            KEEP        cp+1
            KEEP        wn
            KEEP        wm
            KEEP        wm+1
            KEEP        wo
            KEEP        wo+1
            KEEP        wq
            KEEP        wp
            stx         fsp
            cpx         #FSTKMAX
            jcs         over
            jsr         seq_head
            sta         wn
            ldx         pos
            stx         wq
            lda         bitlo,x
            sta         wm
            lda         bithi,x
            sta         wm+1
item:       lda         wn
            beq         done
            jsr         scan
            lda         wo
            ora         wo+1
            beq         fail
            lda         wo
            sta         wm
            lda         wo+1
            sta         wm+1
            ldx         wq              ; the next item starts at the soonest of them
low:        lda         bitlo,x
            and         wm
            bne         :+
            lda         bithi,x
            and         wm+1
            bne         :+
            inx
            bra         low
:           stx         wq
            dec         wn
            jsr         skip
            bra         item
done:       lda         wm
            sta         res
            lda         wm+1
            sta         res+1
            bra         out
fail:       stz         res
            stz         res+1
out:        ldx         fsp
            TAKE        wp
            TAKE        wq
            TAKE        wo+1
            TAKE        wo
            TAKE        wm+1
            TAKE        wm
            TAKE        wn
            TAKE        cp+1
            TAKE        cp
            stx         fsp
            rts
over:       jmp         abort
.endproc

; An alternation: every option from the position (a large one by its index)
.proc e_alt
            ldx         fsp
            KEEP        cp
            KEEP        cp+1
            KEEP        wn
            KEEP        wo
            KEEP        wo+1
            KEEP        wp
            stx         fsp
            cpx         #FSTKMAX
            bcs         over
            jsr         seq_head
            sta         wn
            cmp         #ALTMIN
            bcc         linear
            ldx         nindex
find:       dex
            bmi         linear
            lda         ixnlo,x
            cmp         np
            bne         find
            lda         ixnhi,x
            cmp         np+1
            bne         find
            jsr         alt_ix
            bra         out
linear:     lda         pos
            sta         wp
            stz         wo
            stz         wo+1
loop:       lda         wn
            beq         fin
            jsr         child
            dec         wn
            jsr         skip
            bra         loop
fin:        lda         wo
            sta         res
            lda         wo+1
            sta         res+1
out:        ldx         fsp
            TAKE        wp
            TAKE        wo+1
            TAKE        wo
            TAKE        wn
            TAKE        cp+1
            TAKE        cp
            stx         fsp
            rts
over:       jmp         abort
.endproc

; alt_ix: indexed alternation .X from position pos: the options its first word begins (a binary search of the sorted
; words), and those that begin with an open name the token fits or can match nothing, gathered on fstk, then each
; tried.  (e_alt keeps cp, wn, wo and wp for it.)
.proc alt_ix
            stx         t3
            lda         pos
            sta         wp
            stz         wo
            stz         wo+1
            stz         wn              ; the options gathered
            lda         ixalo,x         ; cp: the options' offsets
            sta         cp
            lda         ixahi,x
            sta         cp+1
            ldy         wp
            cpy         ntok
            bcc         :+
            jmp         wild
:           lda         tokwh,y
            cmp         #$FF
            bne         :+
            jmp         wild
:           sta         bw+1
            lda         tokwl,y
            sta         bw
            stz         blo
            stz         blo+1
            lda         ixclo,x
            sta         bhi
            lda         ixchi,x
            sta         bhi+1
search:     lda         blo+1           ; while lo < hi
            cmp         bhi+1
            bcc         half
            bne         found
            lda         blo
            cmp         bhi
            bcs         found
half:       clc                         ; mid = (lo + hi) / 2
            lda         blo
            adc         bhi
            sta         bmid
            lda         blo+1
            adc         bhi+1
            ror         a
            sta         bmid+1
            ror         bmid
            lda         bmid            ; tp: words + 2 mid
            asl         a
            sta         tp
            lda         bmid+1
            rol         a
            sta         tp+1
            ldx         t3
            clc
            lda         tp
            adc         ixwlo,x
            sta         tp
            lda         tp+1
            adc         ixwhi,x
            sta         tp+1
            ldy         #1              ; words[mid] < w?
            lda         (tp),y
            cmp         bw+1
            bcc         less
            bne         more
            lda         (tp)
            cmp         bw
            bcc         less
more:       lda         bmid
            sta         bhi
            lda         bmid+1
            sta         bhi+1
            bra         search
less:       clc
            lda         bmid
            adc         #1
            sta         blo
            lda         bmid+1
            adc         #0
            sta         blo+1
            bra         search
found:      lda         blo             ; tp: words + 2 lo; tr: their options + lo
            asl         a
            sta         tp
            lda         blo+1
            rol         a
            sta         tp+1
            ldx         t3
            clc
            lda         tp
            adc         ixwlo,x
            sta         tp
            lda         tp+1
            adc         ixwhi,x
            sta         tp+1
            clc
            lda         blo
            adc         ixolo,x
            sta         tr
            lda         blo+1
            adc         ixohi,x
            sta         tr+1
each:       ldx         t3              ; while lo < count and words[lo] = w
            lda         blo+1
            cmp         ixchi,x
            bcc         :+
            bne         wild
            lda         blo
            cmp         ixclo,x
            bcs         wild
:           lda         (tp)
            cmp         bw
            bne         wild
            ldy         #1
            lda         (tp),y
            cmp         bw+1
            bne         wild
            lda         (tr)
            jsr         gather
            clc
            lda         tp
            adc         #2
            sta         tp
            bcc         :+
            inc         tp+1
:           inc         tr
            bne         :+
            inc         tr+1
:           inc         blo
            bne         each
            inc         blo+1
            bra         each
wild:       ldx         t3              ; the options that begin with an open name, or can match nothing
            lda         ixplo,x
            sta         tp
            lda         ixphi,x
            sta         tp+1
            lda         ixpn,x
            sta         t1
            beq         try
wloop:      ldy         #2
            lda         (tp),y
            bmi         take
            ldx         wp
            cpx         ntok
            bcs         wnext
            and         tokmhi,x
            bne         take
            ldy         #1
            lda         (tp),y
            and         tokmlo,x
            beq         wnext
take:       lda         (tp)
            jsr         gather
wnext:      clc
            lda         tp
            adc         #3
            sta         tp
            bcc         :+
            inc         tp+1
:           dec         t1
            bne         wloop
try:        lda         wn              ; each option gathered
            beq         fin
            dec         wn
            ldx         fsp
            dex
            lda         fstk,x
            stx         fsp
            stz         tq+1            ; np: the grammar + offsets[option]
            asl         a
            rol         tq+1
            clc
            adc         cp
            sta         tq
            lda         tq+1
            adc         cp+1
            sta         tq+1
            ldy         #0
            clc
            lda         (tq),y
            adc         gbase
            sta         np
            iny
            lda         (tq),y
            adc         gbase+1
            sta         np+1
            lda         wp
            jsr         ends
            lda         res
            tsb         wo
            lda         res+1
            tsb         wo+1
            bra         try
fin:        lda         wo
            sta         res
            lda         wo+1
            sta         res+1
            rts
.endproc

; gather: option .A onto fstk, for alt_ix
.proc gather
            ldx         fsp
            sta         fstk,x
            inx
            stx         fsp
            cpx         #FSTKMAX
            bcs         over
            inc         wn
            rts
over:       jmp         abort
.endproc

; An option: its child's ends, and the position itself
.proc e_opt
            ldx         fsp
            KEEP        wp
            stx         fsp
            cpx         #FSTKMAX
            bcs         over
            lda         pos
            sta         wp
            jsr         opt_head
            lda         wp
            jsr         ends
            ldx         wp
            lda         bitlo,x
            tsb         res
            lda         bithi,x
            tsb         res+1
            ldx         fsp
            TAKE        wp
            stx         fsp
            rts
over:       jmp         abort
.endproc

; A repeat (zero or more): each round from where the last ended, until nothing new (or MAXREP rounds)
.proc e_rep
            ldx         fsp
            KEEP        cp
            KEEP        cp+1
            KEEP        wn
            KEEP        wm
            KEEP        wm+1
            KEEP        wo
            KEEP        wo+1
            KEEP        wq
            KEEP        wp
            KEEP        ws
            KEEP        ws+1
            stx         fsp
            cpx         #FSTKMAX
            jcs         over
            ldy         #1              ; cp: the child
            lda         (np),y
            bmi         big
            lda         #2
            bra         :+
big:        lda         #3
:           clc
            adc         np
            sta         cp
            lda         np+1
            adc         #0
            sta         cp+1
            ldx         pos             ; the frontier (wm) and what's been seen (ws): the position
            stx         wq
            lda         bitlo,x
            sta         wm
            sta         ws
            lda         bithi,x
            sta         wm+1
            sta         ws+1
            lda         #MAXREP
            sta         wn
round:      lda         wm
            ora         wm+1
            beq         done
            lda         wn
            beq         done
            dec         wn
            jsr         scan            ; (wm emptied; the ends in wo)
            lda         ws              ; the new: wo less what's been seen
            eor         #$FF
            and         wo
            sta         wm
            lda         ws+1
            eor         #$FF
            and         wo+1
            sta         wm+1
            lda         wm
            tsb         ws
            lda         wm+1
            tsb         ws+1
            bra         round
done:       lda         ws
            sta         res
            lda         ws+1
            sta         res+1
            ldx         fsp
            TAKE        ws+1
            TAKE        ws
            TAKE        wp
            TAKE        wq
            TAKE        wo+1
            TAKE        wo
            TAKE        wm+1
            TAKE        wm
            TAKE        wn
            TAKE        cp+1
            TAKE        cp
            stx         fsp
            rts
over:       jmp         abort
.endproc

; ---- The calls -----------------------------------------------------------------------------------------------------

; cm_match: line .A/.X: 1 if it parses, 0 if not, -1 ($FF, .X $FF) if it is too long or nests too deep
.proc cm_match
            sta         lnp
            stx         lnp+1
            tsx
            stx         savedsp
            stz         fsp
            jsr         tokenize
            lda         toolong
            bne         long
            clc                         ; the line's generation: the cache emptied every 16 lines
            lda         gen
            adc         #$10
            sta         gen
            bne         :++
            lda         #$FF
            ldx         #CACHE - 1
:           sta         ckp,x
            dex
            bpl         :-
:
            lda         #<linenode
            sta         np
            lda         #>linenode
            sta         np+1
            lda         #0
            jsr         ends
            ldx         ntok
            lda         res
            and         bitlo,x
            bne         yes
            lda         res+1
            and         bithi,x
            bne         yes
            lda         #0
            tax
            rts
yes:        lda         #1
            ldx         #0
            rts
long:       lda         #$FF
            tax
            rts
.endproc

; cm_world: the world's words (cm_g, cm_i, cm_a's lists) marked on the dictionary, the last world's gone.  The lists
; must stay as they are until the next cm_world.
.proc cm_world
            lda         #<lmark
            sta         tp
            lda         #>lmark
            sta         tp+1
            jsr         zeroent
            stz         ndyn
            lda         cm_g
            sta         lnp
            lda         cm_g+1
            sta         lnp+1
            lda         #W_WORLD
            jsr         marklist
            lda         cm_i
            sta         lnp
            lda         cm_i+1
            sta         lnp+1
            lda         #W_NAME
            jsr         marklist
            lda         cm_a
            sta         lnp
            lda         cm_a+1
            sta         lnp+1
            lda         #W_DEITY
            jmp         marklist
.endproc

; marklist: each word of the space-separated list at lnp marked .A; a deity's name the dictionary lacks is added (one
; of the world's own words, after the index's entries)
.proc marklist
            sta         lflag
spaces:     lda         (lnp)
            beq         done
            cmp         #' '
            bne         word
            inc         lnp
            bne         spaces
            inc         lnp+1
            bra         spaces
done:       rts
word:       lda         lnp
            sta         tp
            lda         lnp+1
            sta         tp+1
            ldy         #0
wl:         lda         (lnp),y
            beq         wend
            cmp         #' '
            beq         wend
            iny
            bne         wl
wend:       sty         tl
            tya
            clc
            adc         lnp
            sta         lnp
            bcc         :+
            inc         lnp+1
:           jsr         lookup
            bcc         mark
            lda         lflag
            cmp         #W_DEITY
            bne         spaces
            ldx         ndyn
            cpx         #MAXDYN
            bcs         spaces
            lda         tp
            sta         dynlo,x
            lda         tp+1
            sta         dynhi,x
            lda         tl
            sta         dynlen,x
            lda         th
            sta         dynhash,x
            inc         ndyn
            txa                         ; its entry: after the index's
            clc
            adc         nent
            pha
            lda         nent+1
            adc         #0
            tax
            pla
mark:       clc                         ; lmark[entry] |= the flag
            adc         #<lmark
            sta         tq
            txa
            adc         #>lmark
            sta         tq+1
            lda         lflag
            ora         (tq)
            sta         (tq)
            bra         spaces
.endproc

; cm_init: the tables (cm_g, cm_i, cm_a) read: C = 0; or C = 1 if a table is not a dq0 table, they don't belong
; together, or one is too large for the matcher
.proc cm_init
            jsr         rdgrammar
            bcs         fail
            jsr         rdindex
            bcs         fail
            lda         #<lmark
            sta         tp
            lda         #>lmark
            sta         tp+1
            jsr         zeroent
            stz         ndyn
            jsr         rdaliases
            bcs         fail
            stz         gen             ; the cache empty
            lda         #$FF
            ldx         #CACHE - 1
:           sta         ckp,x
            dex
            bpl         :-
            clc
            rts
fail:       sec
            rts
.endproc

; magic: does the table at tp begin "DQ0", then .A, then version .X?  C clear if so
.proc magic
            ldy         #3
            cmp         (tp),y
            bne         no
            txa
            ldy         #4
            cmp         (tp),y
            bne         no
            ldy         #0
            lda         (tp),y
            cmp         #'D'
            bne         no
            iny
            lda         (tp),y
            cmp         #'Q'
            bne         no
            iny
            lda         (tp),y
            cmp         #'0'
            bne         no
            clc
            rts
no:         sec
            rts
.endproc

; rdgrammar: the counts, past the words, the open names' kinds, and each rule's body (each where the last ended)
.proc rdgrammar
            lda         cm_g
            sta         tp
            sta         gbase
            lda         cm_g+1
            sta         tp+1
            sta         gbase+1
            lda         #'G'
            ldx         #7
            jsr         magic
            jcs         bad
            ldy         #6              ; rules (MAXRULES at most) and names (255 at most)
            lda         (tp),y
            jne         bad
            dey
            lda         (tp),y
            cmp         #MAXRULES + 1
            jcs         bad
            sta         nrules
            ldy         #8
            lda         (tp),y
            bne         bad
            dey
            lda         (tp),y
            sta         nnames
            ldy         #9
            lda         (tp),y
            sta         nwords
            sta         tq
            iny
            lda         (tp),y
            sta         nwords+1
            sta         tq+1
            clc
            lda         tp
            adc         #11
            sta         tp
            bcc         words
            inc         tp+1
words:      lda         tq              ; each word: its length and its letters, 5 bits each
            ora         tq+1
            beq         kinds
            lda         (tp)
            jsr         packsize
            clc
            adc         tp
            sta         tp
            bcc         :+
            inc         tp+1
:           lda         tq
            bne         :+
            dec         tq+1
:           dec         tq
            bra         words
kinds:      lda         tp
            sta         pkind
            lda         tp+1
            sta         pkind+1
            lda         nnames
            sec
            sbc         nrules
            bcc         bad
            clc
            adc         tp
            sta         cp
            lda         tp+1
            adc         #0
            sta         cp+1
            ldx         #0
rules:      cpx         nrules
            bcs         ok
            lda         cp
            sta         rplo,x
            lda         cp+1
            sta         rphi,x
            phx
            jsr         skip
            plx
            inx
            bra         rules
ok:         clc
            rts
bad:        sec
            rts
.endproc

; rdindex: the counts (the grammar's), the rules treated by name, the dictionary's parts, the Bloom rows, the first
; kinds, and each indexed alternation's parts; "then" found, and the line's node made
.proc rdindex
            lda         cm_i
            sta         tp
            lda         cm_i+1
            sta         tp+1
            lda         #'I'
            ldx         #2
            jsr         magic
            jcs         bad
            ldy         #5
            lda         (tp),y
            cmp         nrules
            jne         bad
            iny
            lda         (tp),y
            jne         bad
            iny
            lda         (tp),y
            cmp         nwords
            jne         bad
            iny
            lda         (tp),y
            cmp         nwords+1
            jne         bad
            iny
            lda         (tp),y
            sta         nent
            iny
            lda         (tp),y
            sta         nent+1
            cmp         #>(MAXENT - MAXDYN)   ; (the world's own words after them)
            bcc         :+
            jne         bad
            lda         nent
            cmp         #<(MAXENT - MAXDYN) + 1
            jcs         bad
:           ldy         #11
            lda         (tp),y
            sta         rline
            iny
            lda         (tp),y
            sta         rseq
            iny
            lda         (tp),y
            sta         rquoted
            clc                         ; the permutation, at 14
            lda         tp
            adc         #14
            sta         pperm
            lda         tp+1
            adc         #0
            sta         pperm+1
            clc                         ; the hashes' starts, at 270; the words, 514 bytes on
            lda         tp
            adc         #<270
            sta         dstart
            lda         tp+1
            adc         #>270
            sta         dstart+1
            clc
            lda         dstart
            adc         #<514
            sta         drec
            lda         dstart+1
            adc         #>514
            sta         drec+1
            lda         dstart          ; the flags: after the words (the 257th start is their length)
            sta         tq
            lda         dstart+1
            clc
            adc         #2
            sta         tq+1
            ldy         #0
            clc
            lda         (tq),y
            adc         drec
            sta         dflags
            iny
            lda         (tq),y
            adc         drec+1
            sta         dflags+1
            clc                         ; the Bloom rows, after the flags
            lda         dflags
            adc         nent
            sta         tp
            lda         dflags+1
            adc         nent+1
            sta         tp+1
            ldx         #0
rows:       lda         tp
            sta         brlo,x
            lda         tp+1
            sta         brhi,x
            jsr         pastrules
            inx
            cpx         #8
            bne         rows
            lda         tp              ; the first kinds
            sta         pklo
            lda         tp+1
            sta         pklo+1
            jsr         pastrules
            lda         tp
            sta         pkhi
            lda         tp+1
            sta         pkhi+1
            jsr         pastrules
            lda         (tp)            ; the indexed alternations
            cmp         #MAXINDEX + 1
            bcs         bad
            sta         nindex
            ldy         #1
            jsr         pasty
            ldx         #0
alts:       cpx         nindex
            bcs         then
            jsr         rdalt
            inx
            bra         alts
then:       lda         #<thenword
            sta         tp
            lda         #>thenword
            sta         tp+1
            lda         #4
            sta         tl
            jsr         lookup
            bcs         bad
            sta         thenlo
            stx         thenhi
            cpx         nwords+1        ; (a grammar word)
            bcc         :+
            bne         bad
            cmp         nwords
            bcs         bad
:           lda         #T_NT
            sta         linenode
            lda         rline
            sta         linenode+1
            stz         linenode+2
            clc
            rts
bad:        sec
            rts
.endproc

; pastrules: tp past a row of a byte a rule.  pasty: tp past .Y bytes.  (.X kept)
.proc pastrules
            ldy         nrules
.endproc
.proc pasty
            tya
            clc
            adc         tp
            sta         tp
            bcc         :+
            inc         tp+1
:           rts
.endproc

; rdalt: indexed alternation .X at tp: its node, its options, its first words and their options, its open options
.proc rdalt
            ldy         #0
            clc
            lda         (tp),y
            adc         gbase
            sta         ixnlo,x
            iny
            lda         (tp),y
            adc         gbase+1
            sta         ixnhi,x
            iny
            lda         (tp),y
            sta         t1              ; its options
            ldy         #3
            jsr         pasty
            lda         tp
            sta         ixalo,x
            lda         tp+1
            sta         ixahi,x
            lda         t1              ; past their offsets
            stz         t2
            asl         a
            rol         t2
            jsr         past16
            ldy         #0
            lda         (tp),y
            sta         ixclo,x
            iny
            lda         (tp),y
            sta         ixchi,x
            ldy         #2
            jsr         pasty
            lda         tp
            sta         ixwlo,x
            lda         tp+1
            sta         ixwhi,x
            lda         ixchi,x         ; past the words
            sta         t2
            lda         ixclo,x
            asl         a
            rol         t2
            jsr         past16
            lda         tp
            sta         ixolo,x
            lda         tp+1
            sta         ixohi,x
            lda         ixchi,x         ; past their options
            sta         t2
            lda         ixclo,x
            jsr         past16
            lda         (tp)            ; the open options
            sta         ixpn,x
            ldy         #1
            jsr         pasty
            lda         tp
            sta         ixplo,x
            lda         tp+1
            sta         ixphi,x
            lda         ixpn,x          ; past them: 3 bytes each
            sta         t1
            stz         t2
            asl         a
            rol         t2
            clc
            adc         t1
            bcc         :+
            inc         t2
:           jmp         past16
.endproc

; past16: tp past .A (lo) and t2 (hi) bytes (.X kept)
.proc past16
            clc
            adc         tp
            sta         tp
            lda         t2
            adc         tp+1
            sta         tp+1
            rts
.endproc

; rdaliases: each default alias: its first word's entry heads its chain; the rest of its key, whether it stands
; anywhere, and its verb's entry kept
.proc rdaliases
            lda         cm_a
            sta         tp
            lda         cm_a+1
            sta         tp+1
            lda         #'A'
            ldx         #1
            jsr         magic
            bcs         bad
            ldy         #5
            lda         (tp),y
            cmp         #MAXALIAS + 1
            bcs         bad
            sta         nalias
            clc
            lda         tp
            adc         #6
            sta         lnp
            lda         tp+1
            adc         #0
            sta         lnp+1
            lda         #<ahead
            sta         tp
            lda         #>ahead
            sta         tp+1
            jsr         zeroent
            stz         ai
loop:       lda         ai
            cmp         nalias
            bcc         one
            clc
            rts
bad:        sec
            rts
one:        lda         (lnp)           ; the key: its length and letters
            sta         klen
            clc
            lda         lnp
            adc         #1
            sta         tp
            lda         lnp+1
            adc         #0
            sta         tp+1
            ldy         #0              ; its first word
fw:         cpy         klen
            beq         fwend
            lda         (tp),y
            cmp         #' '
            beq         fwend
            iny
            bra         fw
fwend:      sty         tl
            ldx         ai
            lda         klen            ; the rest, after the space
            sec
            sbc         tl
            beq         norest
            sbc         #1
            sta         arlen,x
            lda         tl
            sec                         ; (and the space)
            adc         tp
            sta         arlo,x
            lda         tp+1
            adc         #0
            sta         arhi,x
            bra         first
norest:     stz         arlen,x
first:      jsr         lookup          ; the first word's entry (the index has every default alias's)
            bcs         bad
            clc                         ; its chain: this alias first
            adc         #<ahead
            sta         tq
            txa
            adc         #>ahead
            sta         tq+1
            ldx         ai
            lda         (tq)
            sta         anext,x
            txa
            ina
            sta         (tq)
            lda         klen            ; past the key
            sec
            adc         lnp
            sta         lnp
            bcc         :+
            inc         lnp+1
:           lda         (lnp)           ; the verb: its length and letters
            sta         tl
            clc
            lda         lnp
            adc         #1
            sta         tp
            lda         lnp+1
            adc         #0
            sta         tp+1
            jsr         lookup          ; (none: $FFFF)
            ldy         ai
            sta         avlo,y
            txa
            sta         avhi,y
            lda         tl              ; past it
            sec
            adc         lnp
            sta         lnp
            bcc         :+
            inc         lnp+1
:           lda         (lnp)           ; stands anywhere?
            ldy         ai
            sta         aany,y
            inc         lnp
            bne         :+
            inc         lnp+1
:           inc         ai
            jmp         loop
.endproc
