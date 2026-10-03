; ****************************************************************************
; srvlib.s - the server library (docs/reimplementation-from-scratch.md, §12.3): every server is built on it, so
; every server is built the same way, and a new one is mostly tables.  A server module includes srvlib.inc at
; the top of its source and this at its end (.include "srvlib.s": its code and data are the module's own), and
; its HYX2 header names srv_serve as the serve entry.
;
; The server gives:
;   srv_tree        its files, SE_SIZE bytes each, entry 0 its root, ending with a name pointer of 0:
;                     SE_NAME (2) the name; SE_PARENT the parent's entry; SE_KIND (SK_*); SE_HANDLER (2);
;                     SE_MODE its permissions (SM_*); SE_AUX its own
;                   SK_DIR   a directory: its entries are those whose parent it is; it reads as stat records
;                   SK_TEXT  a text file: SE_HANDLER makes the text (srv_tputs, srv_tputc, srv_tputdec ...);
;                            srvlib slices it at the read's offset, so a server keeps nothing between reads
;                   SK_CTL   a ctl file: SE_HANDLER is a table of commands (SC_NAME (2), SC_HANDLER (2), ending
;                            with 0); a write is split into words (numbers: decimal or $hex) and the command's
;                            handler called, srv_argn words after it in srv_argp / srv_arg; a bad command is
;                            E_INVAL.  SE_AUX <> 0: the entry of a text file that reads as its state
;                   SK_DATA  anything else: SE_HANDLER is called with .A = the request (R_OPEN, R_READ,
;                            R_WRITE, R_CLUNK; R_DUP: .Y = the fid duplicated), .X = the fid; it moves the data
;                            (CLIENT_READ, CLIENT_WRITE: the request is in TASK_INBOX), sets RQ_DONE, and answers
;                            C = 0, or C = 1 and .A
;                   SK_DYN   a directory whose children are made as it's read (the tasks, the modules): SE_HANDLER
;                            is called with .A = DYN_NAME (its srv_k-th child: C = 0, .A = its id, its name in
;                            srv_dname; C = 1: no more), DYN_FIND (the child named at srv_p: C = 0, .A = its id;
;                            or C = 1) or DYN_IDNAME (child .X's name into srv_dname); SE_AUX is the entry each
;                            child is (its template: parent SE_TEMPLATE), so a child can be a directory of its
;                            own entries.  A node under a child has the child's id: srv_id (and srv_fid_aux)
;                   SK_RAW   a tree of this entry alone (entry 0): every request for the device goes to its handler
;                            (.A = the request), which does it all, its fids its own (a file system's: #f)
;   SRV_FLUSH       (optional, defined before the .include) a routine for R_FLUSH: client .Y forgotten
;   SRV_OPENED      (optional) a routine for each fid made (R_OPEN, R_DUP: srv_rq): .X = it, its entry srv_ent; it
;                   may set srv_fid_aux,X (from the request's spec: which of the device's instances), or refuse
;                   (C = 1, .A = the error).  A data file's handler is told after it
;   SRV_PRE, SRV_POST (optional) routines run before every request, and after it (before the answer goes back)
;   SRV_TREES       (optional) several devices, a tree each: .byte the letter, .word its tree; ending with 0.  A
;                   request's device (RQ_DEV) chooses the tree.  Without it, the one tree is srv_tree
; srvlib gives: srv_serve; the fids (srv_fid_entry, srv_fid_mode, srv_fid_aux: a byte the handler may keep: a node
; under a dynamic child starts with its id); srv_wait_add and srv_wake_all (wait masks); the text helpers.  It
; answers what it doesn't know with E_NOSYS.
;
; It uses: its zero page (srv_*), r0-r3, and these calls: SRV_TAKE, SRV_REPLY, CLIENT_READ, CLIENT_WRITE, WAKE.

.pushseg

.zeropage
srv_ent:    .res        2               ; An entry, as a pointer into srv_tree ...
srv_e:      .res        1               ;   and as its number
srv_p:      .res        2               ; A pointer (names, the path)
srv_q:      .res        2               ; Another
srv_fid:    .res        1               ; The request's fid
srv_k:      .res        1               ; A counter
srv_n:      .res        1               ; Another
srv_tlen:   .res        1               ; The text's length (srv_tputc ...)
srv_argn:   .res        1               ; A ctl command's words after it
srv_x:      .res        1               ; (srv_number's)
srv_t:      .res        2
srv_base:   .res        2               ; The request's device's tree
srv_id:     .res        1               ; The node's id (under a dynamic directory's child), or 0
srv_dyn:    .res        1               ; (srv_idname's: the template looked for)
srv_rq:     .res        1               ; (srv_opened's: the request ...
srv_old:    .res        1               ;   and R_DUP's old fid)

.bss
srv_fid_entry: .res     SRV_FIDS        ; Each fid's entry ($FF: free)
srv_fid_mode:  .res     SRV_FIDS        ;   its open mode
srv_fid_aux:   .res     SRV_FIDS        ;   the handler's byte
srv_text:   .res        SRV_TEXT_MAX + 1 ; A text file's text
srv_stat:   .res        SR_SIZE         ; A stat record
srv_ctl:    .res        SRV_CTL_MAX + 1 ; A ctl write
srv_argp:   .res        SRV_ARGS * 2    ; A ctl command's words after it ...
srv_arg:    .res        SRV_ARGS * 2    ;   and their numbers (0 if they aren't)
srv_dname:  .res        SRV_DNAME_MAX + 1 ; A dynamic child's name (its handler's)
srv_inited: .res        1               ; ($A5: the fids are set up)

.code

; ****************************************************************************
; The serve entry: .Y = the client.  The request taken, done, and its answer back
srv_serve:
            lda         srv_inited                          ; (The first request: no fids open)
            cmp         #$A5
            beq         :+
            jsr         srv_init
:
            jsr         SRV_TAKE                            ; .A = the request
            pha
            jsr         srv_tree_of                         ; Its device's tree
            pla
            bcs         @nodev
            stz         srv_id
.ifdef SRV_PRE
            pha
            jsr         SRV_PRE
            pla
.endif
            pha                                             ; A raw device (SK_RAW): its handler does the
            lda         #0                                  ;   request
            jsr         srv_entry
            ldy         #SE_KIND
            lda         (srv_ent),Y
            cmp         #SK_RAW                             ; (The last kind: C = 1 for it)
            pla
            bcc         :+
            jsr         srv_handler
            bra         @reply
:
            ldx         #SRV_NREQ - 1
:
            cmp         srv_reqs,X
            beq         :+
            dex
            bpl         :-
            lda         #E_NOSYS
            sec
            bra         @reply
:
            txa
            asl
            tax
            jsr         @go
@reply:
.ifdef SRV_POST
            php
            pha
            jsr         SRV_POST
            pla
            plp
.endif
            jmp         SRV_REPLY                           ; (It keeps .A and C)

@go:
            jmp         (srv_reqvec,X)

@nodev:
            lda         #E_NODEV
            sec
            bra         @reply

; srv_base = the tree of the request's device (SRV_TREES), or srv_tree.  OUT: C = 0; or C = 1: not one of ours
srv_tree_of:
.ifdef SRV_TREES
            ldx         #0
@tree:
            lda         SRV_TREES,X
            beq         @none
            cmp         TASK_INBOX + RQ_DEV
            beq         @found
            inx
            inx
            inx
            bra         @tree

@found:
            lda         SRV_TREES + 1,X
            sta         srv_base
            lda         SRV_TREES + 2,X
            sta         srv_base + 1
            clc
            rts

@none:
            sec
            rts
.else
            lda         #<srv_tree
            sta         srv_base
            lda         #>srv_tree
            sta         srv_base + 1
            clc
            rts
.endif

srv_init:
            ldx         #SRV_FIDS - 1
            lda         #$FF
:
            sta         srv_fid_entry,X
            dex
            bpl         :-
            lda         #$A5
            sta         srv_inited
            rts

; ****************************************************************************
; The requests

; R_OPEN: walk the name, take a fid.  Out: RQ_FID, RQ_PERM (the qid type)
srv_open:
            jsr         srv_walk                            ; srv_e: its entry
            bcs         @done
            jsr         srv_qtype                           ; A directory opens for reading only
            beq         :+
            lda         TASK_INBOX + RQ_MODE
            and         #O_RW_MASK
            beq         :+
            lda         #E_ISDIR
            sec
            rts
:
            jsr         srv_allowed                         ; The mode its permissions allow?
            bcs         @done
            jsr         srv_newfid                          ; A fid: the node (and its id)
            bcs         @done
            lda         #R_OPEN
            jmp         srv_opened

@done:
            rts

; Fid srv_fid, just made (by R_OPEN or R_DUP: .A; R_DUP's old fid: srv_old): a data file's handler says, then its
; answer: RQ_FID, RQ_PERM (its qid type)
srv_opened:
            sta         srv_rq
.ifdef SRV_OPENED
            ldx         srv_fid                             ; The server's say, for every fid
            jsr         SRV_OPENED
            bcs         @refused
.endif
            ldy         #SE_KIND                            ; Its handler's say, for data files
            lda         (srv_ent),Y
            cmp         #SK_DATA
            bne         @ok
            lda         srv_rq
            ldx         srv_fid
            ldy         srv_old
            jsr         srv_handler
            bcc         @ok
@refused:
            ldx         srv_fid                             ; (Refused: the fid back)
            pha
            lda         #$FF
            sta         srv_fid_entry,X
            pla
            sec
            rts

@ok:
            lda         srv_fid
            sta         TASK_INBOX + RQ_FID
            jsr         srv_qtype
            sta         TASK_INBOX + RQ_PERM
            clc
            rts

; R_READ
srv_read:
            jsr         srv_getfid
            bcs         @done
            ldy         #SE_KIND
            lda         (srv_ent),Y
            cmp         #SK_DIR
            beq         srv_readdir
            cmp         #SK_DYN
            beq         srv_readdir
            cmp         #SK_TEXT
            beq         srv_readtext
            cmp         #SK_CTL
            beq         @ctl
            lda         #R_READ                             ; SK_DATA
            ldx         srv_fid
            jmp         srv_handler

@ctl:                                                       ; A ctl file reads as its state's text file, if it has one
            ldy         #SE_AUX
            lda         (srv_ent),Y
            beq         @empty
            jsr         srv_entry
            bra         srv_readtext

@empty:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            clc
@done:
            rts

; A text file (entry srv_ent): its text made, and the part the read asks for sent
srv_readtext:
            jsr         srv_maketext
            bcs         @done
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET + 1          ; Past the text: nothing
            ora         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @end
            lda         TASK_INBOX + RQ_OFFSET
            cmp         srv_tlen
            bcs         @end
            sec                                             ; r2: what's left of it, or what's asked if less
            lda         srv_tlen
            sbc         TASK_INBOX + RQ_OFFSET
            sta         r2
            stz         r2 + 1
            lda         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            lda         TASK_INBOX + RQ_COUNT
            cmp         r2
            bcs         :+
            sta         r2
:
            clc                                             ; r0: from the offset
            lda         #<srv_text
            adc         TASK_INBOX + RQ_OFFSET
            sta         r0
            lda         #>srv_text
            adc         #0
            sta         r0 + 1
            jsr         srv_toclient
@end:
            clc
@done:
            rts

; A directory: its entries' stat records, from record offset / SR_SIZE, as many as the count holds
srv_readdir:
            lda         srv_e
            sta         srv_n                               ; (The directory)
            lda         TASK_INBOX + RQ_OFFSET              ; A record's start?
            and         #SR_SIZE - 1
            bne         @inval
            lda         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @end
            lda         TASK_INBOX + RQ_OFFSET + 1          ; The record: offset / 64
            sta         srv_k
            lda         TASK_INBOX + RQ_OFFSET
            asl
            rol         srv_k
            asl
            rol         srv_k                               ; (srv_k: the first record's number)
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_BUF                 ; r1: where the next one goes
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            sta         r1 + 1
@record:
            lda         TASK_INBOX + RQ_COUNT + 1           ; Room for another?  (The count left >= 64)
            bne         :+
            lda         TASK_INBOX + RQ_COUNT
            cmp         #SR_SIZE
            bcc         @end
:
            jsr         srv_nth                             ; The directory's srv_k-th entry
            bcs         @end
            jsr         srv_makestat
            lda         #<srv_stat
            sta         r0
            lda         #>srv_stat
            sta         r0 + 1
            lda         #SR_SIZE
            sta         r2
            stz         r2 + 1
            jsr         CLIENT_WRITE
            clc
            lda         TASK_INBOX + RQ_DONE
            adc         #SR_SIZE
            sta         TASK_INBOX + RQ_DONE
            bcc         :+
            inc         TASK_INBOX + RQ_DONE + 1
:
            clc
            lda         r1
            adc         #SR_SIZE
            sta         r1
            bcc         :+
            inc         r1 + 1
:
            sec
            lda         TASK_INBOX + RQ_COUNT
            sbc         #SR_SIZE
            sta         TASK_INBOX + RQ_COUNT
            bcs         :+
            dec         TASK_INBOX + RQ_COUNT + 1
:
            inc         srv_k
            lda         srv_n                               ; (srv_child moved srv_e)
            sta         srv_e
            bra         @record

@end:
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; R_WRITE
srv_write:
            jsr         srv_getfid
            bcs         @done
            ldy         #SE_KIND
            lda         (srv_ent),Y
            cmp         #SK_CTL
            beq         srv_writectl
            cmp         #SK_DATA
            bne         @perm
            lda         #R_WRITE
            ldx         srv_fid
            jmp         srv_handler

@perm:
            cmp         #SK_DIR
            beq         @isdir
            cmp         #SK_DYN
            beq         @isdir
            lda         #E_PERM
            sec
@done:
            rts

@isdir:
            lda         #E_ISDIR
            sec
            rts

; A ctl write: the text taken, split into words, and its command's handler called
srv_writectl:
            lda         TASK_INBOX + RQ_COUNT + 1
            bne         @toobig
            lda         TASK_INBOX + RQ_COUNT
            cmp         #SRV_CTL_MAX + 1
            bcs         @toobig
            sta         r2
            stz         r2 + 1
            tax
            stz         srv_ctl,X                           ; (Zero-terminated)
            lda         #<srv_ctl
            sta         r0
            lda         #>srv_ctl
            sta         r0 + 1
            lda         TASK_INBOX + RQ_BUF
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            sta         r1 + 1
            jsr         CLIENT_READ
            jsr         srv_words                           ; srv_p: the command; srv_argn, srv_argp, srv_arg
            bcs         @done
            ldy         #SE_HANDLER                         ; The command table
            lda         (srv_ent),Y
            sta         srv_q
            iny
            lda         (srv_ent),Y
            sta         srv_q + 1
@command:
            ldy         #SC_NAME + 1
            lda         (srv_q),Y
            beq         @inval                              ; (The table's end)
            sta         r3 + 1
            dey
            lda         (srv_q),Y
            sta         r3
            jsr         srv_same                            ; (srv_p and r3: the same word?)
            beq         @found
            clc
            lda         srv_q
            adc         #SC_SIZE
            sta         srv_q
            bcc         @command
            inc         srv_q + 1
            bra         @command

@found:
            lda         TASK_INBOX + RQ_COUNT               ; Done: all of it (if the handler says so)
            sta         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            ldy         #SC_HANDLER
            lda         (srv_q),Y
            sta         r3
            iny
            lda         (srv_q),Y
            sta         r3 + 1
            jmp         (r3)

@toobig:
            lda         #E_TOOBIG
            sec
@done:
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; R_STAT: the fid's stat record to the client's buffer
srv_rstat:
            jsr         srv_getfid
            bcs         @done
            jsr         srv_makestat
            lda         #<srv_stat
            sta         r0
            lda         #>srv_stat
            sta         r0 + 1
            lda         TASK_INBOX + RQ_BUF
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            sta         r1 + 1
            lda         #SR_SIZE
            sta         r2
            sta         TASK_INBOX + RQ_DONE
            stz         r2 + 1
            stz         TASK_INBOX + RQ_DONE + 1
            jsr         CLIENT_WRITE
            clc
@done:
            rts

; R_CLUNK: the fid forgotten (a data file's handler told first)
srv_clunk:
            jsr         srv_getfid
            bcs         @done
            ldy         #SE_KIND
            lda         (srv_ent),Y
            cmp         #SK_DATA
            bne         :+
            lda         #R_CLUNK
            ldx         srv_fid
            jsr         srv_handler
:
            ldx         srv_fid
            lda         #$FF
            sta         srv_fid_entry,X
            clc
@done:
            rts

; R_FLUSH: client .Y forgotten (the server's SRV_FLUSH, if it has one)
srv_flush:
.ifdef SRV_FLUSH
            ldy         TASK_INBOX + RQ_CLIENT
            jmp         SRV_FLUSH
.else
            clc
            rts
.endif

; R_DUP: another fid for the fid's node, in the mode asked (a data file's handler says: .A = R_DUP, .X = the new
; fid, .Y = the old).  OUT: RQ_FID = the new fid, RQ_PERM its qid type
srv_dup:
            jsr         srv_getfid
            bcs         @done
            lda         srv_fid
            sta         srv_old
            jsr         srv_newfid
            bcs         @done
            ldy         srv_old
            lda         srv_fid_aux,Y                       ; (Its aux: the old one's)
            sta         srv_fid_aux,X
            lda         #R_DUP
            jmp         srv_opened

@done:
            rts

; ****************************************************************************
; The pieces

; A fid for node srv_e (srv_ent), in the request's mode, its aux the node's id.  OUT: C = 0, .X = srv_fid = it; or
; C = 1, .A = E_NFILE
srv_newfid:
            ldx         #SRV_FIDS - 1
:
            lda         srv_fid_entry,X
            cmp         #$FF
            beq         :+
            dex
            bpl         :-
            lda         #E_NFILE
            sec
            rts
:
            stx         srv_fid
            lda         srv_e
            sta         srv_fid_entry,X
            lda         TASK_INBOX + RQ_MODE
            sta         srv_fid_mode,X
            lda         srv_id
            sta         srv_fid_aux,X
            clc
            rts

; The srv_k-th entry of directory srv_n (srv_e, srv_ent; a dynamic one's child: its template, srv_id its id).
; OUT: C = 0; or C = 1: there isn't one
srv_nth:
            lda         srv_n
            jsr         srv_entry
            ldy         #SE_KIND
            lda         (srv_ent),Y
            cmp         #SK_DYN
            beq         :+
            lda         srv_k
            jmp         srv_child
:
            lda         #DYN_NAME
            jsr         srv_handler                         ; (srv_k: which)
            bcs         @done
            sta         srv_id
            lda         srv_n
            jsr         srv_entry
            ldy         #SE_AUX
            lda         (srv_ent),Y
            jsr         srv_entry
            clc
@done:
            rts

; A template's (srv_e's) child srv_id: its name into srv_dname, from the dynamic directory whose template it is
srv_idname:
            lda         srv_e
            pha
            sta         srv_dyn
            lda         #0                                  ; That directory: the SK_DYN entry with this template
@entry:
            jsr         srv_entry
            ldy         #SE_NAME + 1
            lda         (srv_ent),Y
            beq         @none
            ldy         #SE_KIND
            lda         (srv_ent),Y
            cmp         #SK_DYN
            bne         @next
            ldy         #SE_AUX
            lda         (srv_ent),Y
            cmp         srv_dyn
            beq         @found
@next:
            lda         srv_e
            inc         a
            bra         @entry

@found:
            lda         #DYN_IDNAME
            ldx         srv_id
            jsr         srv_handler
            bcc         @back
@none:
            stz         srv_dname
@back:
            pla
            jmp         srv_entry

; srv_e and srv_ent = the entry the request's name walks to, from the root.  OUT: C = 0; or C = 1, .A = E_NOENT,
; E_NOTDIR
srv_walk:
            lda         #0
            jsr         srv_entry
            lda         #<TASK_PATH
            sta         srv_p
            lda         #>TASK_PATH
            sta         srv_p + 1
@element:
            lda         (srv_p)                             ; Past the slashes
            cmp         #'/'
            bne         :+
            inc         srv_p
            bne         @element
            inc         srv_p + 1
            bra         @element
:
            cmp         #0
            beq         @found
            ldy         #SE_KIND                            ; A name in a directory, so this must be one
            lda         (srv_ent),Y
            cmp         #SK_DYN
            beq         @dyn
            cmp         #SK_DIR
            bne         @notdir
            lda         srv_e
            sta         srv_n
            stz         srv_k
@child:
            lda         srv_k                               ; Each of its entries
            jsr         srv_child
            bcs         @noent
            ldy         #SE_NAME
            lda         (srv_ent),Y
            sta         r3
            iny
            lda         (srv_ent),Y
            sta         r3 + 1
            jsr         srv_same                            ; (The element, at srv_p, ends at / or 0)
            beq         @next
            inc         srv_k
            lda         srv_n
            jsr         srv_entry
            bra         @child

@next:
            ldy         #0                                  ; Past the element
:
            lda         (srv_p),Y
            beq         :+
            cmp         #'/'
            beq         :+
            iny
            bra         :-
:
            tya
            clc
            adc         srv_p
            sta         srv_p
            bcc         @element
            inc         srv_p + 1
            bra         @element

@found:
            clc
            rts

@dyn:                                                       ; A dynamic directory: its handler finds the child;
            lda         srv_e                               ;   the node is its template, with the child's id
            pha
            lda         #DYN_FIND
            jsr         srv_handler
            plx
            bcs         @noent
            sta         srv_id
            txa
            jsr         srv_entry
            ldy         #SE_AUX
            lda         (srv_ent),Y
            jsr         srv_entry
            jmp         @next

@noent:
            lda         #E_NOENT
            sec
            rts

@notdir:
            lda         #E_NOTDIR
            sec
            rts

; Z = 1 if the word at srv_p (ending at a space, /, a new line or 0) is the name at r3 (zero-terminated)
srv_same:
            ldy         #0
@char:
            lda         (r3),Y
            beq         @end
            cmp         (srv_p),Y
            bne         @no
            iny
            bra         @char

@end:
            lda         (srv_p),Y
            beq         @yes
            cmp         #'/'
            beq         @yes
            cmp         #' '
            beq         @yes
            cmp         #LF
            beq         @yes
@no:
            lda         #1                                  ; (Z = 0)
            rts

@yes:
            lda         #0
            rts

; srv_e, srv_ent = entry .A
srv_entry:
            sta         srv_e
            stz         srv_ent + 1
            asl
            rol         srv_ent + 1
            asl
            rol         srv_ent + 1
            asl
            rol         srv_ent + 1
            clc
            adc         srv_base
            sta         srv_ent
            lda         srv_ent + 1
            adc         srv_base + 1
            sta         srv_ent + 1
            rts

; The .A-th entry (0 on) of directory srv_n: srv_e, srv_ent.  OUT: C = 0; or C = 1: there isn't one
srv_child:
            sta         srv_k
            pha
            lda         #0
            jsr         srv_entry
@entry:
            ldy         #SE_NAME + 1                        ; (The table's end)
            lda         (srv_ent),Y
            beq         @none
            ldy         #SE_PARENT
            lda         (srv_ent),Y
            cmp         srv_n
            bne         @next
            lda         srv_e                               ; (The root isn't its own entry)
            cmp         srv_n
            beq         @next
            lda         srv_k
            beq         @found
            dec         srv_k
@next:
            lda         srv_e
            inc         a
            jsr         srv_entry
            bra         @entry

@found:
            pla
            sta         srv_k
            clc
            rts

@none:
            pla
            sta         srv_k
            sec
            rts

; The request's fid: srv_fid, its entry srv_e / srv_ent.  OUT: C = 0; or C = 1, .A = E_BADF
srv_getfid:
            ldx         TASK_INBOX + RQ_FID
            cpx         #SRV_FIDS
            bcs         @badf
            stx         srv_fid
            lda         srv_fid_entry,X
            cmp         #$FF
            beq         @badf
            ldy         srv_fid_aux,X
            sty         srv_id
            jsr         srv_entry
            clc
            rts

@badf:
            lda         #E_BADF
            sec
            rts

; The open mode (TASK_INBOX's RQ_MODE) within entry srv_ent's permissions?  OUT: C = 0; or C = 1, .A = E_PERM
srv_allowed:
            ldy         #SE_MODE
            lda         TASK_INBOX + RQ_MODE
            and         #O_RW_MASK
            cmp         #O_WRITE
            beq         @write
            lda         (srv_ent),Y                         ; (Reading, or both)
            and         #SM_READ
            beq         @perm
            lda         TASK_INBOX + RQ_MODE
            and         #O_RW_MASK
            cmp         #O_RDWR
            bne         @ok
@write:
            lda         (srv_ent),Y
            and         #SM_WRITE
            beq         @perm
@ok:
            clc
            rts

@perm:
            lda         #E_PERM
            sec
            rts

; .A = entry srv_ent's qid type (Z = 1: a file)
srv_qtype:
            ldy         #SE_KIND
            lda         (srv_ent),Y
            cmp         #SK_DIR
            beq         :+
            cmp         #SK_DYN
            beq         :+
            lda         #QT_FILE
            rts
:
            lda         #QT_DIR
            rts

; Entry srv_ent's text file: its text in srv_text, srv_tlen long.  OUT: C = 0; or C = 1, .A: the maker's error
srv_maketext:
            stz         srv_tlen
            ldy         #SE_HANDLER
            lda         (srv_ent),Y
            sta         r3
            iny
            lda         (srv_ent),Y
            sta         r3 + 1
            jmp         (r3)

; Entry srv_ent's stat record, in srv_stat
srv_makestat:
            ldx         #SR_SIZE - 1
:
            stz         srv_stat,X
            dex
            bpl         :-
            ldy         #SE_NAME                            ; Its name (a template's: its child's, from the
            lda         (srv_ent),Y                         ;   dynamic directory's handler)
            sta         r3
            iny
            lda         (srv_ent),Y
            sta         r3 + 1
            ldy         #SE_PARENT
            lda         (srv_ent),Y
            cmp         #SE_TEMPLATE
            bne         :+
            jsr         srv_idname
            lda         #<srv_dname
            sta         r3
            lda         #>srv_dname
            sta         r3 + 1
:
            ldy         #0
:
            lda         (r3),Y
            sta         srv_stat + SR_NAME,Y
            beq         :+
            iny
            cpy         #31
            bne         :-
:
            jsr         srv_qtype                           ; Its qid: type, path (its entry)
            sta         srv_stat + SR_QTYPE
            lda         srv_e
            sta         srv_stat + SR_QPATH
            lda         srv_id
            sta         srv_stat + SR_QPATH + 1
            ldy         #SE_MODE                            ; Its mode (r, w for all three of user, group, other)
            lda         (srv_ent),Y
            sta         r3
            asl
            asl
            asl
            ora         r3
            sta         srv_stat + SR_MODE
            asl
            asl
            asl
            ora         srv_stat + SR_MODE
            sta         srv_stat + SR_MODE
            lda         r3
            lsr
            lsr                                             ; (The user's read bit is bit 8)
            and         #1
            sta         srv_stat + SR_MODE + 1
            lda         srv_stat + SR_QTYPE
            beq         :+
            lda         #DM_DIR
            tsb         srv_stat + SR_MODE + 1
:
            lda         TASK_INBOX + RQ_DEV                 ; Its device, and instance
            sta         srv_stat + SR_DEV
            lda         TASK_INBOX + RQ_SPEC
            sta         srv_stat + SR_INST
            ldy         #SE_KIND                            ; A text file's length: its text's
            lda         (srv_ent),Y
            cmp         #SK_TEXT
            bne         :+
            jsr         srv_maketext
            lda         srv_tlen
            sta         srv_stat + SR_LENGTH
:
            rts

; Call entry srv_ent's handler: .A, .X and .Y as given
srv_handler:
            pha
            phy
            ldy         #SE_HANDLER
            lda         (srv_ent),Y
            sta         r3
            iny
            lda         (srv_ent),Y
            sta         r3 + 1
            ply
            pla
            jmp         (r3)

; r0 (r2 bytes) to the client's buffer (the request's RQ_BUF); RQ_DONE = r2
srv_toclient:
            lda         TASK_INBOX + RQ_BUF
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            sta         r1 + 1
            lda         r2
            sta         TASK_INBOX + RQ_DONE
            lda         r2 + 1
            sta         TASK_INBOX + RQ_DONE + 1
            jmp         CLIENT_WRITE

; The ctl text in srv_ctl: its first word at srv_p (the command), the rest's places in srv_argp and numbers in
; srv_arg, srv_argn of them (each word ended with a 0 in place).  OUT: C = 0; or C = 1, .A = E_INVAL (no command,
; or too many words)
srv_words:
            ldx         #0                                  ; .X: the byte; srv_argn: the words after the first
            lda         #$FF
            sta         srv_argn
@word:
            lda         srv_ctl,X                           ; Past spaces
            beq         @end
            cmp         #' ' + 1
            bcs         :+
            inx
            bra         @word
:
            lda         srv_argn                            ; A word: where it starts
            bpl         :+
            txa                                             ; (The command)
            clc
            adc         #<srv_ctl
            sta         srv_p
            lda         #>srv_ctl
            adc         #0
            sta         srv_p + 1
            bra         @scan
:
            cmp         #SRV_ARGS
            bcs         @inval
            asl
            tay
            txa
            clc
            adc         #<srv_ctl
            sta         srv_argp,Y
            lda         #>srv_ctl
            adc         #0
            sta         srv_argp + 1,Y
            jsr         srv_number                          ; (Its number, if it is one)
@scan:
            inc         srv_argn
            lda         srv_argn                            ; The last word there can be: the rest of the
            cmp         #SRV_ARGS                           ;   line, spaces and all
            beq         @rest
:
            lda         srv_ctl,X                           ; To its end
            beq         @end
            cmp         #' ' + 1
            bcc         :+
            inx
            bra         :-
:
            stz         srv_ctl,X                           ; (Ended in place)
            inx
            bra         @word

@rest:
            lda         srv_ctl,X                           ; (To a control character, or the end)
            cmp         #' '
            bcc         :+
            inx
            bra         @rest
:
            stz         srv_ctl,X
@end:
            lda         srv_argn
            bmi         @inval                              ; (No command)
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; srv_arg + .Y = the number at srv_ctl + .X (decimal, or $hex), or 0.  Keeps .X, .Y
srv_number:
            stx         srv_x
            stz         r3                                  ; (r3: the number so far)
            stz         r3 + 1
            lda         srv_ctl,X
            cmp         #'$'
            beq         @hex
@dec:
            lda         srv_ctl,X
            sec
            sbc         #'0'
            cmp         #10
            bcs         @done
            pha
            asl         r3                                  ; * 10: * 2 ...
            rol         r3 + 1
            lda         r3
            sta         srv_t
            lda         r3 + 1
            sta         srv_t + 1
            asl         r3                                  ;   ... * 8, and the two
            rol         r3 + 1
            asl         r3
            rol         r3 + 1
            clc
            lda         r3
            adc         srv_t
            sta         r3
            lda         r3 + 1
            adc         srv_t + 1
            sta         r3 + 1
            pla                                             ; + the digit
            clc
            adc         r3
            sta         r3
            bcc         :+
            inc         r3 + 1
:
            inx
            bra         @dec

@hex:
            inx
            lda         srv_ctl,X
            jsr         @digit
            bcs         @done
            asl         r3                                  ; * 16, + the digit
            rol         r3 + 1
            asl         r3
            rol         r3 + 1
            asl         r3
            rol         r3 + 1
            asl         r3
            rol         r3 + 1
            ora         r3
            sta         r3
            bra         @hex

@done:
            lda         r3
            sta         srv_arg,Y
            lda         r3 + 1
            sta         srv_arg + 1,Y
            ldx         srv_x
            rts

@digit:                                                     ; C = 0, .A = the value of hex digit .A; or C = 1
            sec
            sbc         #'0'
            cmp         #10
            bcc         @ok
            and         #$DF                                ; (Either case)
            sec
            sbc         #'A' - '0' - 10
            cmp         #10
            bcc         @bad
            cmp         #16
            bcs         @bad
@ok:
            clc
            rts

@bad:
            sec
            rts

; ****************************************************************************
; Text, for the text files' makers: into srv_text (srv_tlen long; it stops at SRV_TEXT_MAX)

; .A
srv_tputc:
            ldx         srv_tlen
            cpx         #SRV_TEXT_MAX
            bcs         :+
            sta         srv_text,X
            inc         srv_tlen
:
            rts

; The string at .A/.X (zero-terminated)
srv_tputs:
            sta         srv_q
            stx         srv_q + 1
            ldy         #0
:
            lda         (srv_q),Y
            beq         :+
            phy
            jsr         srv_tputc
            ply
            iny
            bne         :-
:
            rts

; .A/.X in decimal
srv_tputdec:
            sta         r3
            stx         r3 + 1
            lda         #0                                  ; (A 0 on the stack: the digits' end)
            pha
@digit:
            ldx         #16                                 ; r3 / 10: the remainder in .A
            lda         #0
:
            asl         r3
            rol         r3 + 1
            rol         a
            cmp         #10
            bcc         :+
            sbc         #10
            inc         r3
:
            dex
            bne         :--
            clc
            adc         #'0'
            pha
            lda         r3
            ora         r3 + 1
            bne         @digit
:
            pla
            beq         :+
            jsr         srv_tputc
            bra         :-
:
            rts

; .A as two hex digits
srv_tputhex:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         @nib
            pla
@nib:
            and         #$0F
            cmp         #10
            bcc         :+
            adc         #'a' - '0' - 10 - 1                 ; (C = 1)
:
            adc         #'0'
            jmp         srv_tputc

; ****************************************************************************
; Wait masks: a 16-bit mask of clients waiting for something (bit = task), in the server's memory

; The request's client into the mask at .A/.X
srv_wait_add:
            sta         srv_q
            stx         srv_q + 1
            lda         TASK_INBOX + RQ_CLIENT
            ldy         #0
            cmp         #8
            bcc         :+
            iny
:
            and         #7
            tax
            lda         srv_bit8,X
            ora         (srv_q),Y
            sta         (srv_q),Y
            rts

; Every client in the mask at .A/.X woken, and the mask cleared.  (From an irq entry too)
srv_wake_all:
            sta         srv_q
            stx         srv_q + 1
            ldx         #0
            ldy         #0
            lda         (srv_q),Y
            jsr         @byte
            ldx         #8
            ldy         #1
            lda         (srv_q),Y
            jsr         @byte
            lda         #0
            sta         (srv_q)
            ldy         #1
            sta         (srv_q),Y
            rts

@byte:                                                      ; .A: the byte; .X: its first task
            beq         @done
            lsr
            pha
            bcc         :+
            txa
            jsr         WAKE
:
            inx
            pla
            bra         @byte

@done:
            rts

.rodata
srv_bit8:   .byte       $01, $02, $04, $08, $10, $20, $40, $80
srv_reqs:   .byte       R_OPEN, R_READ, R_WRITE, R_STAT, R_CLUNK, R_FLUSH, R_DUP
SRV_NREQ    = * - srv_reqs
srv_reqvec: .word       srv_open, srv_read, srv_write, srv_rstat, srv_clunk, srv_flush, srv_dup

.popseg
