; ****************************************************************************
; cmdmean.s - what a command means to Dingus Quest Zero, for the 65C02: the line cm_match has just read (its tokens,
; cmdmatch.s's) made the game's command (hub.h's struct cmd: an action, actions.h's VB_*, and its roles) by the game's
; words (games/dq0/commands.words: the ways, the things, the little words ...) and its verbs (commands.verbs: each
; verb's words, the action it makes by the phrase's shape, the thing it takes unsaid, its own value): the verb, the
; old parser's rules (o_parse: games/dq0/parse.c's, till dq0c replaced it) on the tokens after it, each word as typed
; and as its alias's root, then the action its verb makes of what was said.  The host's reference is
; sim/tools/commandwords.js (scan), and its tables are commandwords.js's (encode: DQ0W, in dq0c.dat); this is held to
; it (sim/tools/commandmatch-test.js --mean).
;
; Its calls (the parser's task, dq0c.s, makes them, the words' table in the window: it may be in a bank of its own):
;   cm_winit    the words' table (cm_w, where it stays): C = 0; or C = 1, not one, or not the index's (cm_init's first)
;   cm_mean     the last line cm_match read, its struct cmd into cm_cmd, the character facing .A (a heading, 0-7):
;               the ways as it faces kept so (REL | n: dq0c's asker makes them the compass's as the command's carried
;               out), but where one by the compass is with one as it faces ("north and to my left"): the facing then
;   cm_wthings  the game's things' words (those with a thing: "door", "room", "trap") marked the world's, though some
;               are the grammar's words too: things to the matcher ("kick the door"); after cm_init and each cm_world
;
; Each word (a token, not quoted text, ! or =) has its records: its entry's (the word as typed; or, an alias of
; several words begun there, its root: "look closely"), and its root's; a field of the word's is the first's, or, if
; that has none and the alias was one word, the root's (get).  Its size: some 1.5K of code, 250 bytes of RAM.
; ****************************************************************************

            .setcpu     "65C02"
            .macpack    longbranch
            .export     cm_winit, cm_mean, cm_wthings, cm_w, cm_cmd
            .import     ntok, tokoff, toklen, tokq, toktl, tokth, tokel, tokeh, tokmu, tbuf, nent, lmark

; struct cmd (hub.h): its fields' places, and a thing's (struct cthing) in it
CM_ACTION   = 0
CM_FLAGS    = 2
CM_DIR      = 3
CM_HOW      = 4
CM_PREP     = 5
CM_AUX      = 6
CM_MANNER   = 7
CM_ORDER    = 8
CM_DFLT     = 9
CM_SKILL    = 10
CM_TASK     = 11
CM_N        = 12
CM_OBJ      = 13
CM_OTHER    = 37
CM_SIZE     = 61
T_KIND      = 0
T_SEL       = 1
T_N         = 2
T_LIES      = 3
T_WORD      = 8
WORDMAX     = 15                    ; A word's letters kept (its field's, less the 0)
CF_NOVERB   = $01                   ; (struct cmd's flags: hub.h's CF_*)
CF_HALF     = $02
CF_UNSAID   = $04

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
C_SEL       = 7
C_NUM       = 8
C_MANNER    = 9
C_LITTLE    = 10
NCOL        = 11

; A verb's fields (commandwords.js's ROW), and its flags
R_ALONE     = 0
R_THING     = 1
R_WAY       = 2
R_UNSAID    = 3
R_AUX       = 4
R_SKILL     = 5
R_TASK      = 6
R_FLAGS     = 7
ROWSIZE     = 8
RF_ORDER    = 1                     ; (An ally's request: its order read)

; The special words (commandwords.js's SPECIAL), and a pair's kind
S_WITH      = 1
S_AWAY      = 2
S_BACK      = 3
S_HALF      = 4
S_AROUND    = 5
S_BAR       = 6
S_WAYOUT    = 7
P_SPECIAL   = 1

NCONST      = 30                    ; The constants (commandwords.js's CONSTANTS, in order)
MAXWS       = 15                    ; The words of a line (cmdmatch.s's MAXTOK + 1)

            .zeropage
mp:         .res        2           ; The table read, a list walked
rp:         .res        2           ; An entry's record; the verb's
cq:         .res        2           ; A column

            .bss
cm_w:       .res        2           ; The words' table
cm_cmd:     .res        CM_SIZE     ; The line's struct cmd
kv:         .res        NCONST      ; The game's values (the verbs' numbers; dq0.h's, hub.h's)
v_MOVE      = kv + 0
v_PUT       = kv + 1
v_STOW      = kv + 2
v_DOFF      = kv + 3
v_HANG      = kv + 4
v_DROP      = kv + 5
v_BAR       = kv + 6
v_GET       = kv + 7
v_TURN      = kv + 8
v_FACE      = kv + 9
v_LIGHT     = kv + 10
v_DOUSE     = kv + 11
v_UNLOCK    = kv + 12
v_NAME      = kv + 13
v_COMBINE   = kv + 14
v_WITH      = kv + 15               ; PR_WITH, PR_ON, PR_FROM
v_ON        = kv + 16
v_FROM      = kv + 17
v_WAYOUT    = kv + 18               ; OB_WAY_OUT, OB_DOOR
v_DOOR      = kv + 19
v_UP        = kv + 20               ; GO_UP, GO_DOWN
v_DOWN      = kv + 21
v_SLME      = kv + 22               ; SL_ME, VB_SAY
v_SAY       = kv + 23
v_STAIRS    = kv + 24               ; OB_STAIRS, OB_LADDER, OB_SHAFT
v_LADDER    = kv + 25
v_SHAFT     = kv + 26
v_THROW     = kv + 27               ; throw; HOW_TO, PR_AT
v_HOWTO     = kv + 28
v_AT        = kv + 29
pgwi:       .res        2           ; Each entry's record
colpl:      .res        NCOL        ; Each column
colph:      .res        NCOL
nmulti:     .res        1           ; The verbs of several words
pmulti:     .res        2
ntwo:       .res        1           ; The pairs
ptwo:       .res        2
nrows:      .res        1           ; The verbs (rows), and the prepositions' own actions
prows:      .res        2
nprep:      .res        1
pprep:      .res        2
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
verb:       .res        1           ; The verb (its row's number), an ally's request or not
ordv:       .res        1
first:      .res        1           ; The word straight after it
wi:         .res        1           ; The word being read
dir:        .res        1           ; The way said (the struct's dir or its thing's lies, as it ends)
spc:        .res        1           ; The word's special
wrec:       .res        1           ; The record of the word kept in the thing's word (as typed)
thoff:      .res        1           ; The thing being said (CM_OBJ, or CM_OTHER after its preposition)
around:     .res        1           ; "around" said
me:         .res        1           ; "me" said
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
            ldy         #0              ; "DQ0W", version 2
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
:           lda         (mp)            ; The pairs: six bytes each
            sta         ntwo
            ldy         #1
            jsr         past
            lda         mp
            sta         ptwo
            lda         mp+1
            sta         ptwo+1
            lda         ntwo
            asl         a
            adc         ntwo
            asl         a
            tay
            jsr         past
            lda         (mp)            ; The verbs: ROWSIZE bytes each
            sta         nrows
            ldy         #1
            jsr         past
            lda         mp
            sta         prows
            lda         mp+1
            sta         prows+1
            ldx         nrows
:           beq         :+
            ldy         #ROWSIZE
            jsr         past
            dex
            bra         :-
:           lda         (mp)            ; The prepositions' own actions
            sta         nprep
            ldy         #1
            jsr         past
            lda         mp
            sta         pprep
            lda         mp+1
            sta         pprep+1
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

; rowp: rp the verb's row (its fields: R_*: ROWSIZE bytes a verb, from 1).  Keeps .X, .Y
.proc rowp
            stz         rp+1
            lda         verb
            dea
            asl         a
            rol         rp+1
            asl         a
            rol         rp+1
            asl         a
            rol         rp+1
            clc
            adc         prows
            sta         rp
            lda         rp+1
            adc         prows+1
            sta         rp+1
            rts
.endproc

; prepof: the verb's own action for word .X straight after it (its preposition's: "look through"), 0 none.  OUT: .A,
; Z with it.  Keeps .X
.proc prepof
            lda         pprep
            sta         mp
            lda         pprep+1
            sta         mp+1
            lda         nprep
            sta         cnt
loop:       lda         cnt
            beq         none
            lda         (mp)            ; (Each: the verb, the word's entry, the action)
            cmp         verb
            bne         next
            ldy         #1
            lda         (mp),y
            cmp         wtl,x
            bne         next
            iny
            lda         (mp),y
            cmp         wth,x
            bne         next
            iny
            lda         (mp),y
            rts
next:       lda         mp
            clc
            adc         #4
            sta         mp
            bcc         :+
            inc         mp+1
:           dec         cnt
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

; digits: word .X a number in digits?  C = 1 and .A its value (255 at most), or C = 0.  Keeps .X
.proc digits
            ldy         wtk,x
            lda         toklen,y
            sta         wl
            lda         tokoff,y
            tay
            stz         kk
loop:       lda         tbuf,y
            sec
            sbc         #'0'
            cmp         #10
            bcs         no
            sta         nn
            lda         kk              ; kk * 10 + the digit, 255 at most
            cmp         #26
            bcs         most
            asl         a
            asl         a
            adc         kk
            asl         a
            adc         nn
            bcs         most
            sta         kk
            bra         :+
most:       lda         #255
            sta         kk
:           iny
            dec         wl
            bne         loop
            lda         kk
            sec
            rts
no:         clc
            rts
.endproc

; copyword: word .X as typed (WORDMAX letters at most), 0-ended, at cm_cmd + .A (a thing's word)
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

; setword: word .X the thing's word, its record kept (for "bar")
.proc setword
            lda         yrec,x
            sta         wrec
            lda         #CM_OBJ + T_WORD
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

; cm_mean: the last line's struct cmd into cm_cmd, the character facing .A
.proc cm_mean
            and         #7
            sta         face
            ldx         #CM_SIZE - 1    ; Nothing yet: no action, no way
:           stz         cm_cmd,x
            dex
            bpl         :-
            lda         #NONE
            sta         cm_cmd + CM_DIR
            sta         cm_cmd + CM_OBJ + T_LIES
            sta         cm_cmd + CM_OTHER + T_LIES
            sta         dir
            stz         wrec
            stz         around
            stz         me
            lda         ntok            ; (Quoted text first: said aloud)
            beq         :+
            lda         tokq
            beq         :+
            lda         v_SAY
            sta         cm_cmd + CM_ACTION
            lda         #CF_NOVERB
            sta         cm_cmd + CM_FLAGS
:           jsr         words
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
            sta         verb
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
            sta         verb
            lda         #1
            sta         wi
verbed:     jsr         rowp            ; (A request: its order)
            ldy         #R_FLAGS
            lda         (rp),y
            and         #RF_ORDER
            sta         ordv
            beq         each0
            ldx         #0
            ldy         #C_ORDER
            jsr         get
            sta         cm_cmd + CM_ORDER
            bra         each0
noverb:     lda         v_MOVE          ; (None: a way, or a thing, alone: going there)
            sta         verb
            stz         ordv
            lda         #CF_NOVERB
            tsb         cm_cmd + CM_FLAGS
each0:      lda         wi
            sta         first
each:       ldx         wi              ; Each word after the verb
            cpx         wsn
            jcs         rules
            lda         #CM_OBJ         ; (The thing being said: the other, after its preposition)
            ldy         cm_cmd + CM_OBJ + T_WORD
            beq         :+
            ldy         cm_cmd + CM_PREP
            beq         :+
            lda         #CM_OTHER
:           sta         thoff
            lda         trec,x          ; (A word none of the game's: a number, or a thing's word, the rest passed)
            ora         rrec,x
            jeq         number
            lda         ordv            ; A request's order, the first
            beq         :+
            ldy         #C_ORDER
            jsr         get
            beq         :+
            ldy         cm_cmd + CM_ORDER
            bne         :+
            sta         cm_cmd + CM_ORDER
:           ldy         #C_SPECIAL
            jsr         get
            sta         spc
            cmp         #S_WITH         ; "with": the other thing's
            bne         :+
            lda         v_WITH
            sta         cm_cmd + CM_PREP
            jmp         next
:           lda         #P_SPECIAL      ; "way out"
            jsr         pair
            cmp         #S_WAYOUT
            bne         :+
            lda         v_WAYOUT
            sta         cm_cmd + CM_OBJ + T_KIND
            inc         wi
            jmp         next
:           lda         cm_cmd + CM_OBJ + T_WORD   ; A prep, after a thing said, the first
            ora         cm_cmd + CM_OBJ + T_SEL
            beq         :+
            lda         cm_cmd + CM_PREP
            bne         :+
            ldy         #C_PREP
            jsr         get
            beq         :+
            sta         cm_cmd + CM_PREP
            jmp         next
:           lda         verb            ; "put away": stowed; "put back": put
            cmp         v_PUT
            bne         :++
            lda         spc
            cmp         #S_AWAY
            bne         :+
            lda         v_STOW
            sta         verb
            jmp         next
:           cmp         #S_BACK
            jeq         next
:           ldy         #C_HOW          ; How: up to it, through it
            jsr         get
            beq         :+
            sta         cm_cmd + CM_HOW
            jmp         next
:           lda         spc             ; "half", "around"
            cmp         #S_HALF
            bne         :+
            lda         #CF_HALF
            tsb         cm_cmd + CM_FLAGS
            jmp         next
:           cmp         #S_AROUND
            bne         :+
            sta         around
            jmp         next
:           ldy         #C_SEL          ; Which of several ("me": the character, if nothing else is said)
            jsr         get
            beq         nosel
            sta         kk
            and         #15
            cmp         v_SLME
            bne         :+
            sta         me
            jmp         next
:           ldy         thoff
            sta         cm_cmd + T_SEL,y
            lda         kk
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            sta         cm_cmd + T_N,y
            jmp         next
nosel:      ldy         #C_MANNER       ; How it's done ("carefully", "hard")
            jsr         get
            beq         :+
            tsb         cm_cmd + CM_MANNER
            jmp         next
:           ldy         #C_LITTLE       ; A little word: nothing
            jsr         get
            jne         next
number:     lda         cm_cmd + CM_N   ; A number: in digits, or a word ("one"), the first
            bne         nonum
            jsr         digits
            bcs         :+
            ldy         #C_NUM
            jsr         get
            beq         nonum
:           sta         cm_cmd + CM_N
            jmp         next
nonum:      ldy         #C_DIR          ; A way: with one before, the way between them (but up or down: the last)
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
            sta         cm_cmd + CM_OBJ + T_KIND
            lda         kk
            bpl         next
            lda         cm_cmd + CM_OBJ + T_WORD
            beq         wset
            jsr         ishead
            beq         next
wset:       jsr         setword
            bra         next
word:       lda         cm_cmd + CM_PREP   ; A word not understood: the thing's (its last before a prep its head); the
            cmp         v_WITH          ;   other thing's after a prep (after "it" and a prep, too)
            beq         word2
            lda         cm_cmd + CM_OBJ + T_WORD
            bne         whead
            lda         cm_cmd + CM_PREP
            beq         wset
            lda         cm_cmd + CM_OBJ + T_SEL
            bne         word2
            bra         wset
whead:      jsr         ishead
            bne         wset
word2:      lda         verb            ;   (a name given: the last)
            cmp         v_NAME
            beq         w2
            lda         cm_cmd + CM_OTHER + T_WORD
            bne         next
            lda         cm_cmd + CM_PREP
            bne         w2
            lda         ordv
            bne         w2
            lda         verb
            cmp         v_COMBINE
            bne         next
w2:         lda         #CM_OTHER + T_WORD
            jsr         copyword
next:       inc         wi
            jmp         each
.endproc

; rules: the line's words read, the old parser's rules on what they made; then its action (action)
.proc rules
            lda         cm_cmd + CM_OBJ + T_KIND   ; With a thing, the way is where it lies
            beq         :+
            lda         dir
            sta         cm_cmd + CM_OBJ + T_LIES
            bra         :++
:           lda         dir
            sta         cm_cmd + CM_DIR
:           lda         verb            ; "take off" no thing but a way: going
            cmp         v_DOFF
            bne         :+
            lda         cm_cmd + CM_OBJ + T_KIND
            bne         :+
            lda         dir
            cmp         #NONE
            beq         :+
            lda         v_MOVE
            sta         verb
:           lda         verb            ; Hung: on something
            cmp         v_HANG
            bne         :+
            lda         v_ON
            sta         cm_cmd + CM_PREP
:           lda         verb            ; Stowed in something: put in it
            cmp         v_STOW
            bne         :+
            lda         cm_cmd + CM_OTHER + T_WORD
            beq         :+
            lda         v_PUT
            sta         verb
:           lda         verb            ; Put down: dropped
            cmp         v_PUT
            bne         :+
            lda         cm_cmd + CM_OTHER + T_WORD
            bne         :+
            lda         dir
            cmp         v_DOWN
            bne         :+
            lda         v_DROP
            sta         verb
:           lda         verb            ; A bar put on a door: barred
            cmp         v_PUT
            bne         :+
            lda         cm_cmd + CM_OBJ + T_KIND
            cmp         v_DOOR
            bne         :+
            lda         cm_cmd + CM_OTHER + T_WORD
            bne         :+
            lda         cm_cmd + CM_OBJ + T_WORD
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
            sta         verb
:           lda         cm_cmd + CM_PREP   ; Off a thing from another: taken; from nothing: taken off
            cmp         v_FROM
            bne         from
            lda         verb
            cmp         v_DOFF
            bne         :+
            lda         cm_cmd + CM_OTHER + T_WORD
            beq         from
            lda         v_GET
            sta         verb
            bra         from
:           cmp         v_GET
            bne         from
            lda         cm_cmd + CM_OTHER + T_WORD
            bne         from
            lda         v_DOFF
            sta         verb
from:       lda         cm_cmd + CM_OBJ + T_WORD   ; "turn the torch on", "turn it off", "put it out": lit, doused
            ora         cm_cmd + CM_OBJ + T_SEL
            beq         unlock
            lda         cm_cmd + CM_OTHER + T_WORD
            bne         unlock
            lda         verb
            cmp         v_TURN
            bne         putout
            lda         cm_cmd + CM_PREP
            cmp         v_ON
            bne         :+
            lda         v_LIGHT
            bra         lit
:           cmp         v_FROM
            bne         putout
            lda         v_DOUSE
lit:        sta         verb
            stz         cm_cmd + CM_PREP
putout:     lda         verb
            cmp         v_PUT
            bne         unlock
            lda         cm_cmd + CM_PREP
            cmp         v_FROM
            bne         unlock
            lda         v_DOUSE
            sta         verb
            stz         cm_cmd + CM_PREP
unlock:     lda         verb            ; "pick it up": taken
            cmp         v_UNLOCK
            bne         :+
            lda         cm_cmd + CM_OBJ + T_WORD
            ora         cm_cmd + CM_OBJ + T_SEL
            beq         :+
            lda         cm_cmd + CM_DIR
            cmp         v_UP
            bne         :+
            lda         v_GET
            sta         verb
            lda         #NONE
            sta         cm_cmd + CM_DIR
:           lda         around          ; Turned around: facing back
            beq         :++
            lda         dir
            cmp         #NONE
            bne         :++
            lda         verb
            cmp         v_TURN
            beq         :+
            cmp         v_FACE
            bne         :++
:           lda         #REL | 4
            sta         cm_cmd + CM_DIR
:           lda         verb            ; "call it Grub": the name the other word
            cmp         v_NAME
            bne         :+
            lda         cm_cmd + CM_OTHER + T_WORD
            bne         :+
            jsr         toword2
            stz         cm_cmd + CM_OBJ + T_WORD
:           lda         verb            ; "go down the stairs": a way down them, not to them
            cmp         v_MOVE
            bne         :+
            lda         cm_cmd + CM_OBJ + T_LIES
            jsr         vert
            bne         :+
            lda         cm_cmd + CM_OBJ + T_KIND
            cmp         v_STAIRS
            beq         down
            cmp         v_LADDER
            beq         down
            cmp         v_SHAFT
            bne         :+
down:       lda         cm_cmd + CM_OBJ + T_LIES
            sta         cm_cmd + CM_DIR
:           lda         cm_cmd + CM_PREP   ; With what: the word, not the other thing's (but combined with it)
            cmp         v_WITH
            bne         :+
            lda         verb
            cmp         v_COMBINE
            beq         :+
            lda         cm_cmd + CM_OBJ + T_WORD
            bne         :+
            jsr         toword
            stz         cm_cmd + CM_OTHER + T_WORD
:           lda         verb            ; "throw at the rat": the mark the other, the thing thrown what's in hand
            cmp         v_THROW
            bne         nomark
            lda         cm_cmd + CM_HOW
            cmp         v_HOWTO
            bne         nomark
            lda         cm_cmd + CM_PREP
            bne         nomark
            lda         cm_cmd + CM_OBJ + T_WORD
            ora         cm_cmd + CM_OBJ + T_SEL
            beq         nomark
            lda         cm_cmd + CM_OTHER + T_WORD
            bne         nomark
            ldx         #CM_OTHER - CM_OBJ - 1
:           lda         cm_cmd + CM_OBJ,x
            sta         cm_cmd + CM_OTHER,x
            stz         cm_cmd + CM_OBJ,x
            dex
            bpl         :-
            lda         #NONE
            sta         cm_cmd + CM_OBJ + T_LIES
            lda         v_AT
            sta         cm_cmd + CM_PREP
nomark:     lda         me              ; "me" and nothing else: the character ("examine me")
            beq         action
            lda         cm_cmd + CM_OBJ + T_WORD
            ora         cm_cmd + CM_OBJ + T_KIND
            ora         cm_cmd + CM_OBJ + T_SEL
            bne         action
            lda         cm_cmd + CM_DIR
            cmp         #NONE
            bne         action
            lda         v_SLME
            sta         cm_cmd + CM_OBJ + T_SEL
action:     jsr         rowp            ; The verb's own, and the action it makes of what was said
            ldy         #R_AUX
            lda         (rp),y
            sta         cm_cmd + CM_AUX
            ldy         #R_SKILL
            lda         (rp),y
            sta         cm_cmd + CM_SKILL
            ldy         #R_TASK
            lda         (rp),y
            sta         cm_cmd + CM_TASK
            ldy         #R_UNSAID
            lda         (rp),y
            sta         cm_cmd + CM_DFLT
            stz         pk              ; (Its own preposition straight after it, the little words passed)
            ldx         first
ploop:      cpx         wsn
            bcs         pdone
            jsr         prepof
            bne         pfound
            ldy         #C_LITTLE
            jsr         get
            beq         pdone
            inx
            bra         ploop
pfound:     sta         pk
pdone:      lda         cm_cmd + CM_DIR    ; Up or down the stairs: the way's
            cmp         #NONE
            beq         :+
            lda         verb
            cmp         v_MOVE
            bne         :+
            lda         cm_cmd + CM_OBJ + T_KIND
            beq         :+
            ldy         #R_WAY
            lda         (rp),y
            bra         set
:           lda         cm_cmd + CM_OBJ + T_WORD   ; A thing said: its preposition's, else the thing's
            ora         cm_cmd + CM_OBJ + T_KIND
            ora         cm_cmd + CM_OBJ + T_SEL
            beq         nothing
            lda         pk
            bne         set
            ldy         #R_THING
            lda         (rp),y
            bne         set
            ldy         #R_ALONE
            lda         (rp),y
            bne         set
            ldy         #R_WAY
            lda         (rp),y
            bra         set
nothing:    lda         cm_cmd + CM_DIR    ; A way: the way's
            cmp         #NONE
            beq         alone
            ldy         #R_WAY
            lda         (rp),y
            bne         set
            ldy         #R_THING
            lda         (rp),y
            bne         set
            ldy         #R_ALONE
            lda         (rp),y
            bra         set
alone:      ldy         #R_ALONE        ; Nothing: the verb's alone; else the thing's, unsaid (its default the UI's)
            lda         (rp),y
            bne         set
            lda         #CF_UNSAID
            tsb         cm_cmd + CM_FLAGS
            ldy         #R_THING
            lda         (rp),y
            bne         set
            ldy         #R_WAY
            lda         (rp),y
set:        sta         cm_cmd + CM_ACTION
            rts
.endproc

; ishead: is a later word the thing's head (no prep yet, nor a request, a name or things combined)?  Z clear if so.
; Keeps .X
.proc ishead
            lda         cm_cmd + CM_PREP
            bne         no
            lda         ordv
            bne         no
            lda         verb
            cmp         v_NAME
            beq         no
            cmp         v_COMBINE
            beq         no
            lda         #1
            rts
no:         lda         #0
            rts
.endproc

; toword2, toword: the thing's word into the other's, or the other's into the thing's
.proc toword2
            ldy         #0
:           lda         cm_cmd + CM_OBJ + T_WORD,y
            sta         cm_cmd + CM_OTHER + T_WORD,y
            iny
            cpy         #WORDMAX + 1
            bne         :-
            rts
.endproc
.proc toword
            ldy         #0
:           lda         cm_cmd + CM_OTHER + T_WORD,y
            sta         cm_cmd + CM_OBJ + T_WORD,y
            iny
            cpy         #WORDMAX + 1
            bne         :-
            rts
.endproc

            .rodata
magic:      .byte       "DQ0W", 2
