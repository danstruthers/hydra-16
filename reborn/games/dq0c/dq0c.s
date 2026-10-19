; ****************************************************************************
; dq0c.s - Dingus Quest Zero's command parser: a task of its own, started by the UI (dq0c SEG ROLE LIBDIR: the hub's
; segment, its role there, hub.h, and the game's lib folder).  As it starts it reads its tables into its RAM
; (LIBDIR/dq0c.dat: the grammar, its index and the default aliases, sim/tools/commandtables.js's, made by make.js);
; then it answers the UI's requests (the hub's cq, hub.h's protocol: posted, the parser's doorbell rung; answered,
; the asker's rung) as they come: a line matched (CQ_MATCH: the reply's res 1, 0, or -1), the world's words set
; (CQ_WORLD), nothing (CQ_NONE: the round trip alone, timed).  Between them it waits on its doorbell, costing
; nothing; it ends when the hub's quit is set (the world's, as the game ends: the UI rings it then).  Assembly
; throughout (cmdmatch.s the matcher): no C runtime.  Its RAM: the tables (17K), the matcher's (4K), a line, the
; world's words.
; ****************************************************************************

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "hub.inc"

            .import     cm_init, cm_world, cm_match, cm_g, cm_i, cm_a

            HYX2_PROGRAM "dq0c", main

TABLES_MAX  = 18432                 ; dq0c.dat's room: its header, then the tables (make.js holds the file to it)
DAT_HEAD    = 11                    ; Its header: "DQ0C", version 1, the grammar's, index's and aliases' lengths
DAT_VERSION = 1
WORLD_MAX   = 1024                  ; The world's words kept (their three lists, each 0-ended)
LINE_MAX    = 255                   ; A line's letters kept

.zeropage
ap:         .res        2           ; Where it reads (the arguments) or writes (the tables, the text)
sp:         .res        2           ; Where the text comes from

.bss
seg:        .res        1           ; The hub's segment, and this task's role in it
role:       .res        1
u0:         .res        1           ; Bank 0's U and bank register (SEG_MAP's), and the text bank's
x0:         .res        1
uc:         .res        1
xc:         .res        1
fd:         .res        1
num:        .res        1
tmp:        .res        1
cnt:        .res        2           ; A count: the bytes to copy, or to read
path:       .res        80
line:       .res        LINE_MAX + 3
world:      .res        WORLD_MAX + 3
tables:     .res        TABLES_MAX

.rodata
datname:    .byte       "/dq0c.dat", 0
nohub:      .byte       "dq0c: no hub", 0
notables:   .byte       "dq0c: no dq0c.dat, or not right", 0
badtables:  .byte       "dq0c: the tables don't fit", 0

.code
main:
            MOVR        ap, r0
            jsr         argnum          ; The segment, the role, the lib folder (then "/dq0c.dat")
            sta         seg
            jsr         argnum
            sta         role
            ldy         #0
:           lda         (ap),y
            beq         :+
            sta         path,y
            iny
            cpy         #64
            bcc         :-
:           ldx         #0
:           lda         datname,x
            sta         path,y
            beq         :+
            iny
            inx
            bra         :-
:           lda         seg             ; The hub: attached, its bank 0 at $8000, its text bank found
            CALL        SEG_ATTACH
            bcs         hubfail
            lda         seg
            ldx         #0
            CALL        SEG_MAP
            bcs         hubfail
            sta         u0
            stx         x0
            jsr         map0
            lda         HUB_WINDOW
            cmp         #<HUB_MAGIC
            bne         hubfail
            lda         HUB_WINDOW + 1
            cmp         #>HUB_MAGIC
            bne         hubfail
            lda         HUB_WINDOW + 2
            cmp         #HUB_VERSION
            bne         hubfail
            lda         seg
            ldx         HUB_WINDOW + HUBO_CQ + CQO_BANK
            CALL        SEG_MAP
            bcs         hubfail
            sta         uc
            stx         xc
            jsr         map0
            CALL        GETPID          ; (This task's number: its role's, in the hub)
            ldx         role
            sta         HUB_WINDOW + HUBO_TASK,x
            jsr         load            ; The tables
            bcs         tabfail
            jsr         cm_init
            bcs         tabbad
loop:       jsr         map0            ; The requests, as they come
            lda         HUB_WINDOW + HUBO_QUIT
            bne         done
            lda         HUB_WINDOW + HUBO_CQ + RQ_STATE
            cmp         #REQ_POSTED
            beq         serve
            ldx         role            ; Asleep till rung (a while at most: the quit looked for between)
            lda         HUB_WINDOW + HUBO_SEM,x
            ldx         #255
            CALL        SEM_WAIT
            bra         loop
done:       lda         #0
            rts
hubfail:    LDR         r0, nohub
            bra         fail
tabfail:    LDR         r0, notables
            bra         fail
tabbad:     LDR         r0, badtables
fail:       lda         #1
            CALL        EXITS

; serve: the request posted answered: its result in res, REQ_DONE, the asker rung
serve:      lda         HUB_WINDOW + HUBO_CQ + RQ_KIND
            cmp         #CQ_MATCH
            beq         match
            cmp         #CQ_WORLD
            beq         words
            lda         #0              ; (CQ_NONE, or one it doesn't know)
            bra         reply
match:      LDR         ap, line        ; A line: matched
            lda         #<LINE_MAX
            ldx         #>LINE_MAX
            jsr         text
            lda         #<line
            ldx         #>line
            jsr         cm_match
            bra         reply
words:      LDR         ap, world       ; The world's words: its three lists, each after the last's 0
            lda         #<WORLD_MAX
            ldx         #>WORLD_MAX
            jsr         text
            LDR         cm_g, world
            MOVR        ap, cm_g
            jsr         pastz
            MOVR        cm_i, ap
            jsr         pastz
            MOVR        cm_a, ap
            jsr         cm_world
            lda         #0
reply:      pha
            jsr         map0
            pla
            sta         HUB_WINDOW + HUBO_CQ + CQO_RES
            lda         #REQ_DONE
            sta         HUB_WINDOW + HUBO_CQ + RQ_STATE
            ldx         role
            inc         HUB_WINDOW + HUBO_BEAT,x
            ldx         HUB_WINDOW + HUBO_CQ + RQ_FROM
            lda         HUB_WINDOW + HUBO_SEM,x
            CALL        SEM_RELEASE
            jmp         loop

; map0, mapc: the hub's bank 0, or its text bank, at $8000 (U, then the bank register: hub.c's hub_map)
map0:       lda         u0
            sta         HUB_U
            lda         x0
            sta         HUB_BANKREG
            rts
mapc:       lda         uc
            sta         HUB_U
            lda         xc
            sta         HUB_BANKREG
            rts

; text: the request's text (its len bytes, .A/.X at most) from the text bank's $8000 into ap, three 0s after it (so a
; list or line is always ended, and the world's three are there)
.proc text
            sta         cnt
            stx         cnt+1
            lda         HUB_WINDOW + HUBO_CQ + CQO_LEN + 1   ; The fewer of len and the most
            cmp         cnt+1
            bcc         len
            bne         most
            lda         HUB_WINDOW + HUBO_CQ + CQO_LEN
            cmp         cnt
            bcs         most
len:        lda         HUB_WINDOW + HUBO_CQ + CQO_LEN
            sta         cnt
            lda         HUB_WINDOW + HUBO_CQ + CQO_LEN + 1
            sta         cnt+1
most:       jsr         mapc
            LDR         sp, HUB_WINDOW
            ldy         #0
copy:       lda         cnt
            ora         cnt+1
            beq         ends
            lda         (sp),y
            sta         (ap),y
            iny
            bne         :+
            inc         sp+1
            inc         ap+1
:           lda         cnt
            bne         :+
            dec         cnt+1
:           dec         cnt
            bra         copy
ends:       lda         #0
            sta         (ap),y
            iny
            bne         :+
            inc         ap+1
:           sta         (ap),y
            iny
            bne         :+
            inc         ap+1
:           sta         (ap),y
            jmp         map0
.endproc

; pastz: ap past the 0 that ends the string there
.proc pastz
            ldy         #0
:           lda         (ap),y
            beq         :+
            iny
            bne         :-
            inc         ap+1
            bra         :-
:           iny
            tya
            clc
            adc         ap
            sta         ap
            bcc         :+
            inc         ap+1
:           rts
.endproc

; argnum: the decimal number at ap (an argument), ap past its 0.  OUT: .A
.proc argnum
            stz         num
            ldy         #0
digit:      lda         (ap),y
            beq         end
            sec
            sbc         #'0'
            sta         tmp
            lda         num             ; num * 10 + the digit
            asl         a
            asl         a
            clc
            adc         num
            asl         a
            clc
            adc         tmp
            sta         num
            iny
            bra         digit
end:        jsr         pastz
            lda         num
            rts
.endproc

; load: dq0c.dat (path) read into tables, its header held to what it should be, and cm_g, cm_i and cm_a set at its
; three tables: C = 0; or C = 1 (none, or not right: .A 1 for the second)
.proc load
            LDR         r0, path
            lda         #O_READ
            CALL        OPEN
            CHECK       none
            sta         fd
            LDR         ap, tables
            LDR         cnt, TABLES_MAX
read:       MOVR        r0, ap          ; As much as there's room for, a read at a time till it ends
            MOVR        r1, cnt
            lda         fd
            CALL        READ
            bcs         shut
            sta         tmp
            stx         num
            ora         num
            beq         shut
            clc
            lda         ap
            adc         tmp
            sta         ap
            lda         ap+1
            adc         num
            sta         ap+1
            sec
            lda         cnt
            sbc         tmp
            sta         cnt
            lda         cnt+1
            sbc         num
            sta         cnt+1
            ora         cnt
            bne         read
shut:       lda         fd
            CALL        CLOSE
            lda         tables          ; "DQ0C", its version
            cmp         #'D'
            bne         bad
            lda         tables + 1
            cmp         #'Q'
            bne         bad
            lda         tables + 2
            cmp         #'0'
            bne         bad
            lda         tables + 3
            cmp         #'C'
            bne         bad
            lda         tables + 4
            cmp         #DAT_VERSION
            bne         bad
            LDR         cm_g, tables + DAT_HEAD   ; The grammar, then the index, then the aliases
            clc
            lda         cm_g
            adc         tables + 5
            sta         cm_i
            lda         cm_g+1
            adc         tables + 6
            sta         cm_i+1
            clc
            lda         cm_i
            adc         tables + 7
            sta         cm_a
            lda         cm_i+1
            adc         tables + 8
            sta         cm_a+1
            clc
            rts
bad:        lda         #1
none:       sec
            rts
.endproc
