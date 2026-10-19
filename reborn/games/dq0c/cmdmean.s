; ****************************************************************************
; cmdmean.s - what a command means to Dingus Quest Zero, for the 65C02: the line cm_match has just read (its tokens,
; cmdmatch.s's) made the game's struct pcmd (dq0.h) by the game's words (games/dq0/commands.words: the verbs, the
; ways, the things, the little words ...), the old parser's rules (o_parse: games/dq0/parse.c's, till dq0c replaced
; it) on the tokens, each word as typed and as its alias's root.  The host's reference is sim/tools/commandwords.js
; (scan), and its tables are commandwords.js's (encode: DQ0W, in dq0c.dat); this is held to it
; (sim/tools/commandmatch-test.js --mean).
;
; Its calls (the parser's task, dq0c.s, makes them):
;   cm_winit    the words' table (cm_w, where it stays): C = 0; or C = 1, not one, or not the index's (cm_init's first)
;   cm_mean     the last line cm_match read, its struct pcmd into cm_cmd, the character facing .A (a heading, 0-7):
;               the ways as it faces kept so (REL | n: dq0c's asker makes them the compass's as the command's carried
;               out), but where one by the compass is with one as it faces ("north and to my left"): the facing then
;   cm_wthings  the game's things' words (those with a thing: "door", "room", "trap") marked the world's, though some
;               are the grammar's words too: things to the matcher ("kick the door"); after cm_init and each cm_world
;
; Each word (a token, not quoted text, ! or =) has its records: its entry's (the word as typed; or, an alias of
; several words begun there, its root: "look closely"), and its root's; a field of the word's is the first's, or, if
; that has none and the alias was one word, the root's (get).  Its size: some 1K of code, 200 bytes of RAM.
; ****************************************************************************

            .setcpu     "65C02"
            .macpack    longbranch
            .export     cm_winit, cm_mean, cm_wthings, cm_w, cm_cmd
            .import     ntok, tokoff, toklen, tokq, toktl, tokth, tokel, tokeh, tokmu, tbuf, nent, lmark

; struct pcmd (dq0.h): its fields' places
PC_VERB     = 0
PC_DIR      = 1
PC_OBJ      = 2
PC_OREL     = 3
PC_HOW      = 4
PC_HALF     = 5
PC_AROUND   = 6
PC_WORD     = 7
PC_PREP     = 23
PC_WORD2    = 24
PC_ORDER    = 40
PC_SIZE     = 41
WORDMAX     = 15                    ; A word's letters kept (its field's, less the 0)

REL         = $10                   ; A way as the character faces (commandwords.js's REL, PART, NONE)
PART        = $80                   ; A thing's part ("the bars": the window's)
NONE        = $FF                   ; No way
W_WORLD     = 1                     ; An entry's mark: the world names a thing by it (cmdmatch.s's)

; The records' columns (commandwords.js's COLUMNS): 0 in each, none (the way's its value + 1)
C_VERB      = 0
C_DIR       = 1
C_THING     = 2
C_PREP      = 3
C_HOW       = 4
C_SPECIAL   = 5
C_ORDER     = 6
C_STYLE     = 7
C_LITTLE    = 8
NCOL        = 9

; The special words (commandwords.js's SPECIAL), and a pair's kind
S_WITH      = 1
S_AWAY      = 2
S_BACK      = 3
S_HALF      = 4
S_AROUND    = 5
S_BAR       = 6
S_WAYOUT    = 7
P_SPECIAL   = 1
P_STYLE     = 2

NCONST      = 25                    ; The constants (commandwords.js's CONSTANTS, in order)
MAXWS       = 15                    ; The words of a line (cmdmatch.s's MAXTOK + 1)

            .zeropage
mp:         .res        2           ; The table read, a list walked
rp:         .res        2           ; An entry's record
cq:         .res        2           ; A column

            .bss
cm_w:       .res        2           ; The words' table
cm_cmd:     .res        PC_SIZE     ; The line's struct pcmd
kv:         .res        NCONST      ; The game's values (dq0.h's, hub.h's)
v_MOVE      = kv + 0
v_TELL      = kv + 1
v_PUT       = kv + 2
v_STOW      = kv + 3
v_DOFF      = kv + 4
v_HANG      = kv + 5
v_DROP      = kv + 6
v_BAR       = kv + 7
v_GET       = kv + 8
v_TURN      = kv + 9
v_FACE      = kv + 10
v_LIGHT     = kv + 11
v_DOUSE     = kv + 12
v_UNLOCK    = kv + 13
v_NAME      = kv + 14
v_COMBINE   = kv + 15
v_ATTACK    = kv + 16
v_WITH      = kv + 17               ; PR_WITH, PR_ON, PR_FROM
v_ON        = kv + 18
v_FROM      = kv + 19
v_WAYOUT    = kv + 20               ; OB_WAY_OUT, OB_DOOR
v_DOOR      = kv + 21
v_UP        = kv + 22               ; GO_UP, GO_DOWN
v_DOWN      = kv + 23
v_BEST      = kv + 24               ; SK_BEST
pgwi:       .res        2           ; Each entry's record
colpl:      .res        NCOL        ; Each column
colph:      .res        NCOL
nmulti:     .res        1           ; The verbs of several words
pmulti:     .res        2
ntwo:       .res        1           ; The pairs
ptwo:       .res        2
face:       .res        1           ; The way the character faces
wsn:        .res        1           ; The line's words: each one's token, its entry (its own, or its alias's root,
wtk:        .res        MAXWS       ;   as above), whether its alias was several words, its records (as typed, its
wtl:        .res        MAXWS       ;   root's, the one its fields are first looked for in)
wth:        .res        MAXWS
wmu:        .res        MAXWS
yrec:       .res        MAXWS
rrec:       .res        MAXWS
trec:       .res        MAXWS
el:         .res        1           ; recof's entry
eh:         .res        1
wi:         .res        1           ; The word being read
dir:        .res        1           ; The way said (the struct's dir or orel, as it ends)
spc:        .res        1           ; The word's special
wrec:       .res        1           ; The record of the word kept in word (as typed)
kk:         .res        1           ; A value for the moment
cnt:        .res        1
nn:         .res        1
pk:         .res        1
xb:         .res        1
yb:         .res        1
fb:         .res        1
wd:         .res        1
wl:         .res        1

            .code

; cm_winit: the words' table at cm_w read: its constants kept, its parts found
.proc cm_winit
            lda         cm_w
            sta         mp
            lda         cm_w+1
            sta         mp+1
            ldy         #0              ; "DQ0W", version 1
:           lda         (mp),y
            cmp         magic,y
            jne         bad
            iny
            cpy         #5
            bne         :-
            ldx         #0              ; The constants
:           lda         (mp),y
            sta         kv,x
            iny
            inx
            cpx         #NCONST
            bne         :-
            lda         (mp),y          ; The entries: as many as the index's
            cmp         nent
            jne         bad
            iny
            lda         (mp),y
            cmp         nent+1
            jne         bad
            iny
            jsr         past
            lda         mp
            sta         pgwi
            lda         mp+1
            sta         pgwi+1
            clc                         ; The records: their count, then each column
            lda         mp
            adc         nent
            sta         mp
            lda         mp+1
            adc         nent+1
            sta         mp+1
            lda         (mp)
            sta         nn
            ldy         #1
            jsr         past
            ldx         #0
col:        lda         mp
            sta         colpl,x
            lda         mp+1
            sta         colph,x
            ldy         nn
            jsr         past
            inx
            cpx         #NCOL
            bne         col
            lda         (mp)            ; The verbs of several words: each its count, its entries, its verb
            sta         nmulti
            sta         cnt
            ldy         #1
            jsr         past
            lda         mp
            sta         pmulti
            lda         mp+1
            sta         pmulti+1
:           lda         cnt
            beq         :+
            lda         (mp)
            asl         a
            clc
            adc         #2
            tay
            jsr         past
            dec         cnt
            bra         :-
:           lda         (mp)            ; The pairs
            sta         ntwo
            ldy         #1
            jsr         past
            lda         mp
            sta         ptwo
            lda         mp+1
            sta         ptwo+1
            clc
            rts
bad:        sec
            rts
.endproc

; cm_wthings: each entry whose record has a thing marked W_WORLD (its mark in lmark)
.proc cm_wthings
            lda         pgwi            ; rp each entry's record, mp its mark, cq the things' column
            sta         rp
            lda         pgwi+1
            sta         rp+1
            lda         #<lmark
            sta         mp
            lda         #>lmark
            sta         mp+1
            lda         colpl + C_THING
            sta         cq
            lda         colph + C_THING
            sta         cq+1
            lda         nent
            sta         el
            lda         nent+1
            sta         eh
loop:       lda         el
            ora         eh
            beq         done
            lda         (rp)
            beq         next
            tay
            lda         (cq),y
            beq         next
            lda         (mp)
            ora         #W_WORLD
            sta         (mp)
next:       inc         rp
            bne         :+
            inc         rp+1
:           inc         mp
            bne         :+
            inc         mp+1
:           lda         el
            bne         :+
            dec         eh
:           dec         el
            bra         loop
done:       rts
.endproc

; past: mp past .Y bytes
.proc past
            tya
            clc
            adc         mp
            sta         mp
            bcc         :+
            inc         mp+1
:           rts
.endproc

; recof: entry el/eh's record (0: none: no entry, or a word of the world's own, past the index's).  Keeps .X, .Y
.proc recof
            lda         eh
            cmp         nent+1
            bcc         in
            bne         none
            lda         el
            cmp         nent
            bcs         none
in:         clc
            lda         el
            adc         pgwi
            sta         rp
            lda         eh
            adc         pgwi+1
            sta         rp+1
            lda         (rp)
            rts
none:       lda         #0
            rts
.endproc

; get: word .X's field .Y: its first record's, or (none there, and its alias not several words) its root's.  OUT: .A
; (0: none), Z with it.  Keeps .X
.proc get
            lda         colpl,y
            sta         cq
            lda         colph,y
            sta         cq+1
            ldy         trec,x
            lda         (cq),y
            bne         done
            lda         wmu,x
            bne         none
            ldy         rrec,x
            lda         (cq),y
done:       rts
none:       lda         #0
            rts
.endproc

; pair: words .X and .X + 1 together, a pair of the kind .A: its value (0: none).  OUT: .A, Z with it.  Keeps .X
.proc pair
            sta         pk
            inx
            cpx         wsn
            dex
            bcs         none
            lda         ptwo
            sta         mp
            lda         ptwo+1
            sta         mp+1
            lda         ntwo
            sta         cnt
            ldy         #0
loop:       lda         cnt
            beq         none
            sty         yb
            lda         (mp),y          ; (Each: the two entries, the kind, the value)
            cmp         wtl,x
            bne         next
            iny
            lda         (mp),y
            cmp         wth,x
            bne         next
            iny
            lda         (mp),y
            cmp         wtl+1,x
            bne         next
            iny
            lda         (mp),y
            cmp         wth+1,x
            bne         next
            iny
            lda         (mp),y
            cmp         pk
            bne         next
            iny
            lda         (mp),y
            rts
next:       lda         yb
            clc
            adc         #6
            tay
            dec         cnt
            bra         loop
none:       lda         #0
            rts
.endproc

; words: the line's words (its tokens but quoted text, ! and =), each one's token, entry, records
.proc words
            ldx         #0              ; .X the token, .Y the word
            ldy         #0
loop:       cpx         ntok
            bcs         done
            lda         tokq,x
            bne         skip
            lda         toklen,x
            cmp         #1
            bne         take
            phy
            ldy         tokoff,x
            lda         tbuf,y
            ply
            cmp         #'!'
            beq         skip
            cmp         #'='
            beq         skip
take:       txa
            sta         wtk,y
            lda         tokmu,x
            sta         wmu,y
            lda         tokel,x         ; Its root's record
            sta         el
            lda         tokeh,x
            sta         eh
            jsr         recof
            sta         rrec,y
            lda         toktl,x         ; As typed
            sta         el
            lda         tokth,x
            sta         eh
            jsr         recof
            sta         yrec,y
            lda         wmu,y           ; Its entry: an alias of several words's root, else as typed
            beq         typed
            lda         rrec,y
            sta         trec,y
            lda         tokel,x
            sta         wtl,y
            lda         tokeh,x
            sta         wth,y
            bra         next
typed:      lda         yrec,y
            sta         trec,y
            lda         toktl,x
            sta         wtl,y
            lda         tokth,x
            sta         wth,y
next:       iny
skip:       inx
            bra         loop
done:       sty         wsn
            rts
.endproc

; copyword: word .X as typed (WORDMAX letters at most), 0-ended, at cm_cmd + .A (PC_WORD or PC_WORD2)
.proc copyword
            sta         wd
            ldy         wtk,x
            lda         toklen,y
            cmp         #WORDMAX + 1
            bcc         :+
            lda         #WORDMAX
:           sta         wl
            lda         tokoff,y
            tay
            ldx         wd
loop:       lda         wl
            beq         end
            lda         tbuf,y
            sta         cm_cmd,x
            iny
            inx
            dec         wl
            bra         loop
end:        stz         cm_cmd,x
            rts
.endproc

; setword: word .X the struct's word, its record kept (for "bar")
.proc setword
            lda         yrec,x
            sta         wrec
            lda         #PC_WORD
            jmp         copyword
.endproc

; between: the way between dir and kk, neither up nor down (a quarter apart: the eighth between them; else kk), one by
; the compass and one as the character faces: that one made the compass's first.  OUT: .A
.proc between
            lda         dir
            eor         kk
            and         #REL
            beq         same
            lda         dir
            and         #REL
            beq         kfirst
            lda         dir
            clc
            adc         face
            and         #7
            sta         dir
            bra         same
kfirst:     lda         kk
            clc
            adc         face
            and         #7
            sta         kk
same:       lda         dir
            and         kk
            and         #REL
            sta         fb
            lda         dir
            and         #7
            sta         xb
            lda         kk
            and         #7
            sta         yb
            sec                         ; kk two eighths after dir: the one between
            sbc         xb
            and         #7
            cmp         #2
            bne         :+
            lda         xb
            bra         mid
:           lda         xb              ; or before it
            sec
            sbc         yb
            and         #7
            cmp         #2
            bne         :+
            lda         yb
mid:        ina
            and         #7
            ora         fb
            rts
:           lda         yb
            ora         fb
            rts
.endproc

; vert: .A up or down?  Z if so
.proc vert
            cmp         v_UP
            beq         :+
            cmp         v_DOWN
:           rts
.endproc

; cm_mean: the last line's struct pcmd into cm_cmd, the character facing .A
.proc cm_mean
            and         #7
            sta         face
            ldx         #PC_SIZE - 1    ; Nothing yet: no verb, no way
:           stz         cm_cmd,x
            dex
            bpl         :-
            lda         #NONE
            sta         cm_cmd + PC_DIR
            sta         cm_cmd + PC_OREL
            sta         dir
            stz         wrec
            jsr         words
            lda         wsn
            bne         :+
            rts
:           stz         wi              ; The verb, if the line begins with one: of several words first
            lda         pmulti
            sta         mp
            lda         pmulti+1
            sta         mp+1
            lda         nmulti
            sta         cnt
mloop:      lda         cnt
            beq         single
            lda         (mp)
            sta         nn
            lda         wsn
            cmp         nn
            bcc         mnext
            ldx         #0
            ldy         #1
mword:      lda         (mp),y
            cmp         wtl,x
            bne         mnext
            iny
            lda         (mp),y
            cmp         wth,x
            bne         mnext
            iny
            inx
            cpx         nn
            bne         mword
            lda         (mp),y
            sta         cm_cmd + PC_VERB
            stx         wi
            bra         verbed
mnext:      lda         nn
            asl         a
            clc
            adc         #2
            tay
            jsr         past
            dec         cnt
            bra         mloop
single:     ldx         #0              ; Or of one
            ldy         #C_VERB
            jsr         get
            beq         noverb
            sta         cm_cmd + PC_VERB
            lda         #1
            sta         wi
verbed:     lda         cm_cmd + PC_VERB   ; (A request: its order)
            cmp         v_TELL
            bne         each
            ldx         #0
            ldy         #C_ORDER
            jsr         get
            sta         cm_cmd + PC_ORDER
            bra         each
noverb:     lda         v_MOVE          ; (None: a way, or a thing, alone: going there)
            sta         cm_cmd + PC_VERB
each:       ldx         wi              ; Each word after the verb
            cpx         wsn
            jcs         rules
            lda         cm_cmd + PC_VERB   ; A request's order, the first
            cmp         v_TELL
            bne         :+
            ldy         #C_ORDER
            jsr         get
            beq         :+
            ldy         cm_cmd + PC_ORDER
            bne         :+
            sta         cm_cmd + PC_ORDER
:           ldy         #C_SPECIAL
            jsr         get
            sta         spc
            cmp         #S_WITH         ; "with": the other thing's
            bne         :+
            lda         v_WITH
            sta         cm_cmd + PC_PREP
            jmp         next
:           lda         #P_SPECIAL      ; "way out"
            jsr         pair
            cmp         #S_WAYOUT
            bne         :+
            lda         v_WAYOUT
            sta         cm_cmd + PC_OBJ
            inc         wi
            jmp         next
:           lda         cm_cmd + PC_WORD   ; A prep, after a word, the first
            beq         :+
            lda         cm_cmd + PC_PREP
            bne         :+
            ldy         #C_PREP
            jsr         get
            beq         :+
            sta         cm_cmd + PC_PREP
            jmp         next
:           lda         cm_cmd + PC_VERB   ; "put away": stowed; "put back": put
            cmp         v_PUT
            bne         :++
            lda         spc
            cmp         #S_AWAY
            bne         :+
            lda         v_STOW
            sta         cm_cmd + PC_VERB
            jmp         next
:           cmp         #S_BACK
            jeq         next
:           ldy         #C_HOW          ; How: up to it, through it
            jsr         get
            beq         :+
            sta         cm_cmd + PC_HOW
            jmp         next
:           lda         spc             ; "half", "around"
            cmp         #S_HALF
            bne         :+
            lda         #1
            sta         cm_cmd + PC_HALF
            jmp         next
:           cmp         #S_AROUND
            bne         :+
            lda         #1
            sta         cm_cmd + PC_AROUND
            jmp         next
:           ldy         #C_LITTLE       ; A little word: nothing
            jsr         get
            jne         next
            ldy         #C_DIR          ; A way: with one before, the way between them (but up or down: the last)
            jsr         get
            beq         thing
            dea
            sta         kk
            lda         dir
            cmp         #NONE
            beq         newdir
            jsr         vert
            beq         newdir
            lda         kk
            jsr         vert
            beq         newdir
            jsr         between
            sta         dir
            jmp         next
newdir:     lda         kk
            sta         dir
            jmp         next
thing:      ldy         #C_THING        ; A thing (a part of one: its word too, if there's none yet)
            jsr         get
            beq         word
            sta         kk
            and         #$FF ^ PART
            sta         cm_cmd + PC_OBJ
            lda         kk
            bpl         next
            lda         cm_cmd + PC_WORD
            bne         next
            jsr         setword
            bra         next
word:       lda         cm_cmd + PC_WORD   ; The first word not understood; and the other thing's, after a prep
            bne         word2
            lda         cm_cmd + PC_PREP
            cmp         v_WITH
            beq         word2
            jsr         setword
            bra         next
word2:      lda         cm_cmd + PC_VERB   ;   (a name given: the last)
            cmp         v_NAME
            beq         w2
            lda         cm_cmd + PC_WORD2
            bne         next
            lda         cm_cmd + PC_PREP
            bne         w2
            lda         cm_cmd + PC_VERB
            cmp         v_TELL
            beq         w2
            cmp         v_COMBINE
            bne         next
w2:         lda         #PC_WORD2
            jsr         copyword
next:       inc         wi
            jmp         each
.endproc

; rules: the line's words read, the old parser's rules on what they made
.proc rules
            lda         cm_cmd + PC_OBJ    ; With a thing, the way is where it lies
            beq         :+
            lda         dir
            sta         cm_cmd + PC_OREL
            bra         :++
:           lda         dir
            sta         cm_cmd + PC_DIR
:           lda         cm_cmd + PC_VERB   ; "take off" no thing but a way: going
            cmp         v_DOFF
            bne         :+
            lda         cm_cmd + PC_OBJ
            bne         :+
            lda         dir
            cmp         #NONE
            beq         :+
            lda         v_MOVE
            sta         cm_cmd + PC_VERB
:           lda         cm_cmd + PC_VERB   ; Hung: put on; stowed in something: put in it
            cmp         v_HANG
            bne         :+
            lda         v_ON
            sta         cm_cmd + PC_PREP
            bra         put
:           cmp         v_STOW
            bne         :+
            lda         cm_cmd + PC_WORD2
            beq         :+
put:        lda         v_PUT
            sta         cm_cmd + PC_VERB
:           lda         cm_cmd + PC_VERB   ; Put down: dropped
            cmp         v_PUT
            bne         :+
            lda         cm_cmd + PC_WORD2
            bne         :+
            lda         dir
            cmp         v_DOWN
            bne         :+
            lda         v_DROP
            sta         cm_cmd + PC_VERB
:           lda         cm_cmd + PC_VERB   ; A bar put on a door: barred
            cmp         v_PUT
            bne         :+
            lda         cm_cmd + PC_OBJ
            cmp         v_DOOR
            bne         :+
            lda         cm_cmd + PC_WORD2
            bne         :+
            lda         cm_cmd + PC_WORD
            beq         :+
            lda         colpl + C_SPECIAL
            sta         cq
            lda         colph + C_SPECIAL
            sta         cq+1
            ldy         wrec
            lda         (cq),y
            cmp         #S_BAR
            bne         :+
            lda         v_BAR
            sta         cm_cmd + PC_VERB
:           lda         cm_cmd + PC_PREP   ; Off a thing from another: taken; from nothing: taken off
            cmp         v_FROM
            bne         from
            lda         cm_cmd + PC_VERB
            cmp         v_DOFF
            bne         :+
            lda         cm_cmd + PC_WORD2
            beq         from
            lda         v_GET
            sta         cm_cmd + PC_VERB
            bra         from
:           cmp         v_GET
            bne         from
            lda         cm_cmd + PC_WORD2
            bne         from
            lda         v_DOFF
            sta         cm_cmd + PC_VERB
from:       lda         cm_cmd + PC_WORD   ; "turn the torch on", "turn it off", "put it out": lit, doused
            beq         unlock
            lda         cm_cmd + PC_WORD2
            bne         unlock
            lda         cm_cmd + PC_VERB
            cmp         v_TURN
            bne         putout
            lda         cm_cmd + PC_PREP
            cmp         v_ON
            bne         :+
            lda         v_LIGHT
            bra         lit
:           cmp         v_FROM
            bne         putout
            lda         v_DOUSE
lit:        sta         cm_cmd + PC_VERB
            stz         cm_cmd + PC_PREP
putout:     lda         cm_cmd + PC_VERB
            cmp         v_PUT
            bne         unlock
            lda         cm_cmd + PC_PREP
            cmp         v_FROM
            bne         unlock
            lda         v_DOUSE
            sta         cm_cmd + PC_VERB
            stz         cm_cmd + PC_PREP
unlock:     lda         cm_cmd + PC_VERB   ; "pick it up": taken
            cmp         v_UNLOCK
            bne         :+
            lda         cm_cmd + PC_WORD
            beq         :+
            lda         cm_cmd + PC_DIR
            cmp         v_UP
            bne         :+
            lda         v_GET
            sta         cm_cmd + PC_VERB
            lda         #NONE
            sta         cm_cmd + PC_DIR
:           lda         cm_cmd + PC_AROUND ; Turned around: facing back
            beq         :++
            lda         dir
            cmp         #NONE
            bne         :++
            lda         cm_cmd + PC_VERB
            cmp         v_TURN
            beq         :+
            cmp         v_FACE
            bne         :++
:           lda         #REL | 4
            sta         cm_cmd + PC_DIR
:           lda         cm_cmd + PC_VERB   ; "call it Grub": the name the other word
            cmp         v_NAME
            bne         :+
            lda         cm_cmd + PC_WORD2
            bne         :+
            jsr         toword2
            stz         cm_cmd + PC_WORD
:           lda         cm_cmd + PC_PREP   ; With what: the word, not the other thing's (but combined with it)
            cmp         v_WITH
            bne         :+
            lda         cm_cmd + PC_VERB
            cmp         v_COMBINE
            beq         :+
            lda         cm_cmd + PC_WORD
            bne         :+
            jsr         toword
            stz         cm_cmd + PC_WORD2
:           lda         cm_cmd + PC_VERB   ; A blow: struck with what its verb says
            cmp         v_ATTACK
            bne         done
            ldx         #0
            lda         #P_STYLE
            jsr         pair
            bne         :+
            ldy         #C_STYLE
            jsr         get
            bne         :+
            lda         v_BEST
:           sta         cm_cmd + PC_HALF
done:       rts
.endproc

; toword2, toword: the word into word2, or word2 into word
.proc toword2
            ldy         #0
:           lda         cm_cmd + PC_WORD,y
            sta         cm_cmd + PC_WORD2,y
            iny
            cpy         #WORDMAX + 1
            bne         :-
            rts
.endproc
.proc toword
            ldy         #0
:           lda         cm_cmd + PC_WORD2,y
            sta         cm_cmd + PC_WORD,y
            iny
            cpy         #WORDMAX + 1
            bne         :-
            rts
.endproc

            .rodata
magic:      .byte       "DQ0W", 1
