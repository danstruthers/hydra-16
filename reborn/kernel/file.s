; ****************************************************************************
; file.s - files (BIOS ROM page 2: far calls; docs/design/reimplementation-from-scratch.md, §12).
;
; An fd (TA_FD, in the task's OS area) names a channel, in the kernel task: its server's task and device letter,
; the server's fid, its mode and qid type, its offset, and how many fds name it (K_CH_*).  A request is a block in
; the client's TA_REQ (RQ_*: appendix B) and, for a name, its TA_PATH; the call SCALLs the server, whose serve
; entry takes them (SRV_TAKE), answers in registers (C, .A) and puts what it returns (a new fid, the count done)
; back (SRV_REPLY).  Data moves between the client's buffer and the server's memory by kcopy (CLIENT_READ,
; CLIENT_WRITE), with the client waiting in its call: kcopy's safe partner.
;
; Blocking, the same everywhere: a server never waits inside a request.  With nothing for it yet, it answers
; E_AGAIN; the call waits till the server's event count changes from what it was as the server took the request
; (or a WAKE: a wait mask's), and asks again, or answers E_AGAIN itself for a non-blocking fd.  A note ends the wait:
; the server gets R_FLUSH, the call E_INTR.  READ asks at most IO_UNIT at a time, and goes on till the count is
; done, the end of the file, or a short count; WRITE till the count is done.
;
; The channels' fields are read and written with quick looks (the offset after each request; a count of fds
; with an inc or a dec, IRQs off); a channel is taken, and a device found, in KCALLs.
;
; A name is made whole and clean here (the current directory before a relative one; ".", "..", "//"), then found in
; the namespace (ns.s, page 3): its candidates, tried in turn till one is there (a member whose device is gone,
; E_NODEV, is passed over as one without the name: a RAM disk stopped leaves /bin's union working).  A channel
; opened on a mount point with more members than one is a union directory: a READ reads the members in turn.

.include "kdefs.inc"

.segment "KCODE_P2"

; ****************************************************************************
; Opening and closing

; OPEN: open a file.  IN: r0 = its path; .A = the mode (O_*).  OUT: .A = an fd; or C = 1, .A = E_NOENT, E_NODEV,
; E_NAMETOOLONG, E_MFILE (no fd free), E_NFILE (no channel free), or the server's error
K_OPEN:
            ldx         #R_OPEN
            stz         TA_REQ + RQ_PERM
            bra         f_open

; CREATE: make a file, or a directory, and open it.  IN: r0 = its path; .A = the mode; .X = its mode's high byte
; (DM_DIR: a directory).  OUT: as OPEN's
K_CREATE:
            stx         TA_REQ + RQ_PERM
            ldx         #R_CREATE
f_open:
            stx         TA_REQ + RQ_TYPE
            sta         TA_REQ + RQ_MODE
            jsr         f_flags
            jsr         f_name                              ; Its candidates
            bcs         @out
            jsr         f_newfd                             ; An fd ...
            bcs         @out
            KCALL_FAR   K_CH_NEW_K                          ; ... and a channel
            bcc         :+
@out:
            rts
:
            sta         F_CH
@cand:                                                      ; Each candidate, till one opens (a CREATE has one)
            ldx         #0
            lda         TA_REQ + RQ_TYPE
            cmp         #R_CREATE
            bne         :+
            inx
:
            txa
            jsr         f_cand
            bcs         @undo
            jsr         f_zero
            jsr         f_send
            bcc         @opened
            jsr         f_pass                              ; (Not in that member, or its device gone: the next)
            bcc         @cand
            bra         @undo

@opened:
            jsr         f_chset                             ; The channel: what the server answered
            lda         F_EXACT                             ; A mount point with more members than one: a union
            beq         :+                                  ;   directory, its members read in turn
            lda         F_NMEM
            cmp         #2
            bcc         :+
            lda         F_CH
            KCALL_FAR   K_NS_UNION_K
:
            ldx         F_FD
            lda         F_CH
            sta         TA_FD,X
            lda         F_FD
            clc
@done:
            rts

@undo:                                                      ; (The channel back: no fd names it)
            jsr         f_chfree
            sec
            rts

; Channel F_CH free again (no fd names it).  Keeps .A
f_chfree:
            pha
            ldx         F_CH
            ldy         T_REGISTER
            php
            sei
            stz         T_REGISTER
            stz         K_CH_REFS,X
            sty         T_REGISTER
            plp
            pla
            rts

; PIPE: a pipe (#|): two fds, .A to read from and .X to write to.  OUT: or C = 1, .A = E_NODEV (no #|), E_MFILE,
; E_NFILE, or the server's error
K_PIPE:
            lda         #<f_s_pipe
            sta         r0
            lda         #>f_s_pipe
            sta         r0 + 1
            lda         #O_READ                             ; The read end: a new pipe
            jsr         K_OPEN
            bcs         @done
            pha                                             ; (Its fd)
            jsr         f_fd
            jsr         f_chload                            ; (Its server and fid)
            jsr         f_newfd                             ; The write end: an fd ...
            bcs         @undo
            KCALL_FAR   K_CH_NEW_K                          ;   a channel ...
            bcs         @undo
            sta         F_CH
            lda         #R_DUP                              ;   and the pipe's other end (its server's R_DUP)
            sta         TA_REQ + RQ_TYPE
            lda         #O_WRITE
            sta         TA_REQ + RQ_MODE
            jsr         f_flags
            jsr         f_send
            bcs         @free
            jsr         f_chset
            ldx         F_FD
            lda         F_CH
            sta         TA_FD,X
            pla                                             ; (The read end's fd)
            clc
@done:
            rts

@free:
            jsr         f_chfree
@undo:                                                      ; (The read end closed: the error kept)
            plx
            pha
            txa
            jsr         K_CLOSE
            pla
            sec
            rts

; CLOSE: close an fd.  IN: .A = the fd.  OUT: C = 0; or C = 1, .A = E_BADF.  The last fd to close its channel
; clunks the server's fid
K_CLOSE:
            jsr         f_fd
            bcc         :+
            rts
:
            ldx         F_FD
            lda         #$FF
            sta         TA_FD,X
f_release:                                                  ; (Channel F_CH: one fd fewer)
            ldx         F_CH
            ldy         T_REGISTER
            php
            sei
            stz         T_REGISTER
            dec         K_CH_REFS,X
            lda         K_CH_REFS,X
            sty         T_REGISTER
            plp
            cmp         #0
            bne         @kept
            jsr         f_chload                            ; The last: the server forgets the fid
            lda         #R_CLUNK
            sta         TA_REQ + RQ_TYPE
            jsr         f_send
            lda         F_CH                                ; (A union directory's namespace: one user fewer)
            KCALL_FAR   K_NS_UREL_K
@kept:
            clc
            rts

; Every fd closed (a task's end: K_EXITS)
K_CLOSE_ALL:
            lda         #FD_MAX - 1
            sta         F_LEFT
:
            lda         F_LEFT
            jsr         K_CLOSE
            dec         F_LEFT
            bpl         :-
            rts

; DUP: another fd for the same channel (the same offset).  IN: .A = an fd.  OUT: .A = the new fd (the lowest
; free); or C = 1, .A = E_BADF, E_MFILE
K_DUP:
            jsr         f_fd
            bcs         @done
            jsr         f_newfd
            bcs         @done
            jsr         f_share
            lda         F_FD
            clc
@done:
            rts

; DUP2: fd .X for the same channel as fd .A (closed first if it's open).  OUT: C = 0; or C = 1, .A = E_BADF
K_DUP2:
            cpx         #FD_MAX
            bcs         @badf
            phx
            jsr         f_fd
            plx
            bcs         @done
            cpx         F_FD
            beq         @same
            lda         F_CH
            pha
            phx
            txa
            jsr         K_CLOSE                             ; (If it's open)
            plx
            stx         F_FD
            pla
            sta         F_CH
            jsr         f_share
@same:
            clc
@done:
            rts

@badf:
            FAIL        E_BADF

; ****************************************************************************
; Reading and writing

; READ: .A = an fd; r0 = a buffer; r1 = a count.  OUT: .A/.X = the count read (0: the end of the file); or C = 1,
; .A = E_BADF, E_AGAIN (non-blocking, nothing yet), E_INTR (a note), or the server's error
K_READ:
            ldx         #R_READ
            bra         f_rw

; WRITE: .A = an fd; r0 = the data; r1 = its count.  OUT: .A/.X = the count written; or C = 1, as READ's
K_WRITE:
            ldx         #R_WRITE
f_rw:
            stx         F_CHUNK                             ; (The request, a moment)
            jsr         f_fd
            bcc         :+
            rts
:
            jsr         f_chload
            lda         TA_REQ + RQ_MODE                    ; Open for it?  (Reads: O_READ or O_RDWR; writes:
            and         #O_RW_MASK                          ;   O_WRITE or O_RDWR)
            ldx         F_CHUNK
            cpx         #R_READ
            bne         :+
            cmp         #O_WRITE
            beq         @notopen
            bra         @open
:
            cmp         #O_READ
            bne         @open
@notopen:
            lda         #E_BADF
            sec
            rts

@open:
            lda         r1                                  ; The count, the buffer
            sta         F_LEFT
            lda         r1 + 1
            sta         F_LEFT + 1
            lda         r0
            sta         TA_REQ + RQ_BUF
            lda         r0 + 1
            sta         TA_REQ + RQ_BUF + 1
            stz         F_TOTAL
            stz         F_TOTAL + 1
            stx         TA_REQ + RQ_TYPE
@chunk:
            lda         F_LEFT                              ; At most IO_UNIT at a time
            ora         F_LEFT + 1
            beq         @done
            lda         F_LEFT
            sta         F_CHUNK
            lda         F_LEFT + 1
            sta         F_CHUNK + 1
            cmp         #>IO_UNIT
            bcc         :+
            lda         #<IO_UNIT
            sta         F_CHUNK
            lda         #>IO_UNIT
            sta         F_CHUNK + 1
:
            lda         F_CHUNK
            sta         TA_REQ + RQ_COUNT
            lda         F_CHUNK + 1
            sta         TA_REQ + RQ_COUNT + 1
            stz         TA_REQ + RQ_DONE
            stz         TA_REQ + RQ_DONE + 1
            jsr         f_send
            bcs         @error
            jsr         f_moved                             ; The offset, the buffer, the counts on
            lda         TA_REQ + RQ_DONE                    ; Nothing: the end (of the file, or of what's there)
            ora         TA_REQ + RQ_DONE + 1
            beq         @eof
            lda         TA_REQ + RQ_TYPE                    ; A short read: that's all for now (a short write:
            cmp         #R_WRITE                            ;   the rest, in the next request)
            beq         @chunk
            lda         TA_REQ + RQ_DONE
            cmp         F_CHUNK
            lda         TA_REQ + RQ_DONE + 1
            sbc         F_CHUNK + 1
            bcc         @done
            jmp         @chunk

@eof:                                                       ; The end: but a union directory's member's is the next
            lda         TA_REQ + RQ_TYPE                    ;   member's start
            cmp         #R_READ
            bne         @done
            jsr         f_unext
            bcs         @done
            jmp         @chunk

@done:
            lda         F_TOTAL
            ldx         F_TOTAL + 1
            clc
            rts

@error:                                                     ; An error after some was moved: what was moved
            pha
            lda         F_TOTAL
            ora         F_TOTAL + 1
            beq         :+
            pla
            bra         @done
:
            pla
            sec
            rts

@badf:
            FAIL        E_BADF

; Channel F_CH at the end of what it's reading: if it's a union directory, its next member opened in its place, from
; its start (a member that isn't there: the one after it).  OUT: C = 0: read on; C = 1: the end
f_unext:
            lda         F_CH
            KCALL_FAR   K_NS_UNEXT_K                        ; (The next member's name: TA_PATH, RQ_DEV, RQ_SPEC)
            bcs         @end
            pha                                             ; (Its server)
            lda         #R_CLUNK                            ; This member's fid forgotten
            sta         TA_REQ + RQ_TYPE
            jsr         f_send
            pla
@open:
            sta         F_SRV
            lda         #R_OPEN                             ; (In the channel's mode: RQ_MODE, as loaded)
            sta         TA_REQ + RQ_TYPE
            stz         TA_REQ + RQ_PERM
            jsr         f_zero
            jsr         f_send
            bcc         @opened
            cmp         #E_NOENT                            ; (Not in that member, or its device gone: the next)
            beq         :+
            cmp         #E_NODEV
            bne         @dead
:
            lda         F_CH
            KCALL_FAR   K_NS_UNEXT_K
            bcc         @open
@dead:                                                      ; (None opens: the channel's dead)
            lda         #$FF
            sta         F_SRV
            jsr         f_chset
@end:
            sec
            rts

@opened:
            jsr         f_chset
            lda         #R_READ
            sta         TA_REQ + RQ_TYPE
            clc
            rts

; The request's offset and count: 0
f_zero:
            ldx         #3
:
            stz         TA_REQ + RQ_OFFSET,X
            dex
            bpl         :-
            stz         TA_REQ + RQ_COUNT
            stz         TA_REQ + RQ_COUNT + 1
            rts

; RQ_DONE moved: the offset (the request's, and the channel's), the buffer, the total and the count left on
f_moved:
            clc
            lda         TA_REQ + RQ_OFFSET
            adc         TA_REQ + RQ_DONE
            sta         TA_REQ + RQ_OFFSET
            lda         TA_REQ + RQ_OFFSET + 1
            adc         TA_REQ + RQ_DONE + 1
            sta         TA_REQ + RQ_OFFSET + 1
            bcc         :+
            inc         TA_REQ + RQ_OFFSET + 2
            bne         :+
            inc         TA_REQ + RQ_OFFSET + 3
:
            jsr         f_setoff
            clc
            lda         TA_REQ + RQ_BUF
            adc         TA_REQ + RQ_DONE
            sta         TA_REQ + RQ_BUF
            lda         TA_REQ + RQ_BUF + 1
            adc         TA_REQ + RQ_DONE + 1
            sta         TA_REQ + RQ_BUF + 1
            clc
            lda         F_TOTAL
            adc         TA_REQ + RQ_DONE
            sta         F_TOTAL
            lda         F_TOTAL + 1
            adc         TA_REQ + RQ_DONE + 1
            sta         F_TOTAL + 1
            sec
            lda         F_LEFT
            sbc         TA_REQ + RQ_DONE
            sta         F_LEFT
            lda         F_LEFT + 1
            sbc         TA_REQ + RQ_DONE + 1
            sta         F_LEFT + 1
            bcs         :+
            stz         F_LEFT                              ; (More than asked: none left)
            stz         F_LEFT + 1
:
            rts

; SEEK: move an fd's offset.  IN: .A = the fd; r0, r1 = the offset (low word, high word: signed); .X = from where:
; 0 the start, 1 the offset now, 2 the end.  OUT: r0, r1 = the new offset; or C = 1, .A = E_BADF, E_INVAL
K_SEEK:
            cpx         #3
            bcs         @inval
            stx         F_CHUNK
            jsr         f_fd
            bcs         @done
            jsr         f_chload
            ldx         F_CHUNK
            beq         @set
            dex
            beq         @add                                ; (From the offset: RQ_OFFSET, as loaded)
            lda         #<TA_PATH                           ; From the end: its length, from its stat (into
            sta         TA_REQ + RQ_BUF                     ;   TA_PATH: no name goes with it)
            lda         #>TA_PATH
            sta         TA_REQ + RQ_BUF + 1
            lda         #SR_SIZE
            sta         TA_REQ + RQ_COUNT
            stz         TA_REQ + RQ_COUNT + 1
            lda         #R_STAT
            sta         TA_REQ + RQ_TYPE
            jsr         f_send
            bcs         @done
            ldx         #3
:
            lda         TA_PATH + SR_LENGTH,X
            sta         TA_REQ + RQ_OFFSET,X
            dex
            bpl         :-
@add:
            clc
            lda         TA_REQ + RQ_OFFSET
            adc         r0
            sta         r0
            lda         TA_REQ + RQ_OFFSET + 1
            adc         r0 + 1
            sta         r0 + 1
            lda         TA_REQ + RQ_OFFSET + 2
            adc         r1
            sta         r1
            lda         TA_REQ + RQ_OFFSET + 3
            adc         r1 + 1
            sta         r1 + 1
@set:
            lda         r1 + 1                              ; (Before the start: no)
            bmi         @inval
            lda         r0
            sta         TA_REQ + RQ_OFFSET
            lda         r0 + 1
            sta         TA_REQ + RQ_OFFSET + 1
            lda         r1
            sta         TA_REQ + RQ_OFFSET + 2
            lda         r1 + 1
            sta         TA_REQ + RQ_OFFSET + 3
            jsr         f_setoff
            clc
@done:
            rts

@inval:
            FAIL        E_INVAL

; ****************************************************************************
; Stat

; FSTAT: an fd's stat record.  IN: .A = the fd; r0 = a buffer (SR_SIZE bytes).  OUT: C = 0; or C = 1, .A = E_BADF,
; or the server's error
K_FSTAT:
            ldx         #R_STAT
f_fstat:
            stx         F_CHUNK
            jsr         f_fd
            bcs         @done
            jsr         f_chload
            lda         F_CHUNK
            sta         TA_REQ + RQ_TYPE
            lda         r0
            sta         TA_REQ + RQ_BUF
            lda         r0 + 1
            sta         TA_REQ + RQ_BUF + 1
            lda         #SR_SIZE
            sta         TA_REQ + RQ_COUNT
            stz         TA_REQ + RQ_COUNT + 1
            jmp         f_send
@done:
            rts

; FWSTAT: change an fd's file (its name, its mode) from a stat record.  IN: .A = the fd; r0 = the record.
; OUT: as FSTAT's
K_FWSTAT:
            ldx         #R_WSTAT
            bra         f_fstat

; STAT: a file's stat record, by name.  IN: r0 = the path; r1 = a buffer (SR_SIZE bytes).  OUT: as OPEN's errors,
; or FSTAT's
K_STAT:
            ldx         #R_STAT
f_stat:
            stx         F_TOTAL                             ; (The request, a moment)
            lda         r1
            pha
            lda         r1 + 1
            pha
            lda         #O_READ
            jsr         K_OPEN
            plx
            stx         r0 + 1                              ; (r0: the buffer, for FSTAT)
            plx
            stx         r0
            bcs         @done
            pha
            ldx         F_TOTAL
            jsr         f_fstat
            plx                                             ; (The fd)
            php
            pha
            txa
            jsr         K_CLOSE
            pla
            plp
@done:
            rts

; WSTAT: change a file from a stat record, by name.  IN: r0 = the path; r1 = the record.  OUT: as STAT's
K_WSTAT:
            ldx         #R_WSTAT
            bra         f_stat

; REMOVE: remove a file (or an empty directory).  IN: r0 = the path.  OUT: C = 0; or C = 1, as OPEN's errors, or
; the server's
K_REMOVE:
            lda         #R_REMOVE
            sta         TA_REQ + RQ_TYPE
            stz         TA_REQ + RQ_MODE
            jsr         f_flags
            jsr         f_name
            bcs         @done
@cand:                                                      ; Each candidate, till one has it
            lda         #0
            jsr         f_cand
            bcs         @done
            jsr         f_send
            bcc         @done
            jsr         f_pass                              ; (Not in that member, or its device gone: the next)
            bcc         @cand
@done:
            rts

; ****************************************************************************
; The pieces

; Send TA_REQ to the server, F_SRV, and wait for its answer: E_AGAIN means wait and ask again (or, non-blocking,
; give E_AGAIN back): till the server's event count changes from what it was as it took the request (RQ_EVENT), or
; a WAKE (a wait mask's); a note ends it (R_FLUSH to the server, E_INTR).  OUT: C = 0; or C = 1, .A = the error
f_send:
            ldx         T_REGISTER                          ; The client: this task, its U, its note group
            stx         TA_REQ + RQ_CLIENT
            lda         U_REGISTER
            sta         TA_REQ + RQ_U
            php
            sei
            stz         T_REGISTER
            lda         K_NGROUP,X
            stx         T_REGISTER
            plp
            sta         TA_REQ + RQ_GROUP
@ask:
            ldy         F_SRV
            cpy         #TASKS
            bcs         @nodev                              ; (Its server has ended)
            cpy         T_REGISTER
            beq         @busy                               ; (A server isn't its own client)
            stz         TK_WOKEN                            ; (A wake from here on: its wait won't wait)
            lda         TA_REQ + RQ_TYPE
            FARCALL     K_SCALL
            bcc         @done
            cmp         #E_AGAIN
            bne         @fail
            lda         TA_REQ + RQ_FLAGS
            and         #RF_NONBLOCK
            bne         @again
            lda         TK_NOTED
            bne         @intr
            lda         F_SRV
            ldx         TA_REQ + RQ_EVENT
            FARCALL     K_EVWAIT
            lda         TK_NOTED
            beq         @ask
@intr:
            lda         #R_FLUSH                            ; A note: the server forgets this client
            sta         TA_REQ + RQ_TYPE
            ldy         F_SRV
            FARCALL     K_SCALL
            lda         #E_INTR
            sec
            rts

@again:
            lda         #E_AGAIN
@fail:
            sec
@done:
            rts

@nodev:
            FAIL        E_NODEV

@busy:
            FAIL        E_BUSY

; RQ_FLAGS from the mode in RQ_MODE
f_flags:
            stz         TA_REQ + RQ_FLAGS
            lda         TA_REQ + RQ_MODE
            and         #O_NONBLOCK
            beq         :+
            lda         #RF_NONBLOCK
            sta         TA_REQ + RQ_FLAGS
:
            rts

; The name at r0 (in this task's view), made whole and clean (f_path), and found in the namespace (ns.s): its
; candidates come from f_cand.  OUT: F_NMEM = how many, F_EXACT <> 0 if it's a mount point itself; or C = 1, .A =
; E_NOENT, E_NAMETOOLONG
f_name:
            stz         F_GONE
            jsr         f_path
            bcs         @done
            KCALL_FAR   K_NS_FIND_K
            bcs         @done
            sta         F_NMEM
            stx         F_EXACT
@done:
            rts

; The name's next candidate: its server (F_SRV), its path in it (TA_PATH, RQ_NAMELEN), its device and spec
; (RQ_DEV, RQ_SPEC).  IN: .A = 0, or 1 for a CREATE's (one only).  OUT: .X = its flags (MCREATE); or C = 1, .A =
; E_NOENT (no more), E_NODEV, E_NAMETOOLONG
f_cand:
            KCALL_FAR   K_NS_NEXT_K
            bcs         :+
            sta         F_SRV
            rts
:
            cmp         #E_NOENT                            ; (No more: E_NODEV if every member that answered had
            bne         :+                                  ;   its device gone)
            ldx         F_GONE
            cpx         #2
            bne         :+
            lda         #E_NODEV
:
            sec
            rts

; A candidate's error .A: C = 0 if it's one to pass over (E_NOENT: the name isn't in it; E_NODEV: its device is
; gone, as a RAM disk stopped), noted in F_GONE; else C = 1 (.A kept)
f_pass:
            cmp         #E_NOENT
            beq         @not
            cmp         #E_NODEV
            beq         @gone
            sec
            rts

@gone:
            lda         #2
            bra         @note

@not:
            lda         #1
@note:
            tsb         F_GONE
            clc
            rts

; The name at r0 into TA_PATH, whole and clean: a # name as it is; an absolute one; a relative one after the current
; directory (TA_CWD).  Then cleaned (f_clean).  OUT: C = 0; or C = 1, .A = E_NOENT (empty), E_NAMETOOLONG
f_path:
            ldx         #0
            lda         (r0)
            beq         @noent
            cmp         #'/'
            beq         @name
            cmp         #'#'
            beq         @name
@cwd:                                                       ; Relative: the directory, a /, then it
            lda         TA_CWD,X
            beq         @slash
            sta         TA_PATH,X
            inx
            cpx         #PATH_MAX
            bcc         @cwd
            bra         @long

@slash:
            lda         #'/'
            sta         TA_PATH,X
            inx
@name:
            ldy         #0
:
            cpx         #PATH_MAX + 1
            bcs         @long
            lda         (r0),Y
            sta         TA_PATH,X
            beq         f_clean
            iny
            inx
            bra         :-

@long:
            FAIL        E_NAMETOOLONG

@noent:
            FAIL        E_NOENT

; TA_PATH cleaned in place, from its root (/, or #x and its spec, as Plan 9's #I1: #c2 is the console's window 2): no
; empty elements, no ".", and ".." taking the element before it (never the root).  OUT: C = 0; or C = 1, .A = E_NOENT
; (a # with no device letter, or a spec longer than 8)
f_clean:
            ldx         #1                                  ; (.X: where the next byte goes; .Y: the next one read)
            lda         TA_PATH
            cmp         #'#'
            bne         @root
            lda         TA_PATH + 1                         ; #x, its spec, then nothing or a /
            beq         @noent
            inx
:
            lda         TA_PATH,X
            beq         @root
            cmp         #'/'
            beq         @root
            inx
            cpx         #2 + 8 + 1
            bcc         :-
            bra         @noent

@root:
            stx         F_ROOT
            txa
            tay
@element:                                                   ; The start of an element, past its /s
            lda         TA_PATH,Y
            beq         @end
            cmp         #'/'
            bne         @name
            iny
            bra         @element

@name:
            cmp         #'.'                                ; "." or ".."?
            bne         @copy
            lda         TA_PATH + 1,Y
            beq         @dot
            cmp         #'/'
            beq         @dot
            cmp         #'.'
            bne         @copy
            lda         TA_PATH + 2,Y
            beq         @dotdot
            cmp         #'/'
            bne         @copy
@dotdot:                                                    ; "..": back over the element before it (to its /, which
            iny                                             ;   the next element's own takes the place of), but not
            iny                                             ;   past the root
:
            cpx         F_ROOT
            beq         @element
            dex
            lda         TA_PATH,X
            cmp         #'/'
            bne         :-
            bra         @element

@dot:
            iny
            bra         @element

@noent:
            FAIL        E_NOENT

@copy:                                                      ; An element: a / before it (but right after the root's /)
            lda         TA_PATH - 1,X
            cmp         #'/'
            beq         :+
            lda         #'/'
            sta         TA_PATH,X
            inx
:
            lda         TA_PATH,Y
            beq         @end
            cmp         #'/'
            beq         @element
            sta         TA_PATH,X
            inx
            iny
            bra         :-

@end:
            stz         TA_PATH,X
            clc
            rts

; F_FD = the lowest free fd.  OUT: C = 0; or C = 1, .A = E_MFILE
f_newfd:
            ldx         #0
:
            lda         TA_FD,X
            cmp         #CH_MAX
            bcs         :+
            inx
            cpx         #FD_MAX
            bne         :-
            FAIL        E_MFILE
:
            stx         F_FD
            clc
            rts

; Fd .A open?  OUT: F_FD = it, F_CH = its channel; or C = 1, .A = E_BADF.  Modifies .X
f_fd:
            cmp         #FD_MAX
            bcs         @badf
            sta         F_FD
            tax
            lda         TA_FD,X
            cmp         #CH_MAX
            bcs         @badf
            sta         F_CH
            clc
            rts

@badf:
            FAIL        E_BADF

; Fd F_FD names channel F_CH too: one fd more
f_share:
            ldx         F_FD
            lda         F_CH
            sta         TA_FD,X
            tax
            ldy         T_REGISTER
            php
            sei
            stz         T_REGISTER
            inc         K_CH_REFS,X
            sty         T_REGISTER
            plp
            rts

; Channel F_CH into the request: its server (F_SRV), device, fid, mode (and RQ_FLAGS), offset
f_chload:
            ldx         F_CH
            ldy         T_REGISTER
            php
            sei
            stz         T_REGISTER                          ; ---- The kernel task's channels
            lda         K_CH_SRV,X
            sty         T_REGISTER
            sta         F_SRV
            stz         T_REGISTER
            lda         K_CH_DEV,X
            sty         T_REGISTER
            sta         TA_REQ + RQ_DEV
            stz         T_REGISTER
            lda         K_CH_FID,X
            sty         T_REGISTER
            sta         TA_REQ + RQ_FID
            stz         T_REGISTER
            lda         K_CH_MODE,X
            sty         T_REGISTER
            sta         TA_REQ + RQ_MODE
            plp
            php
            sei
            stz         T_REGISTER
            lda         K_CH_OFF0,X
            sty         T_REGISTER
            sta         TA_REQ + RQ_OFFSET
            stz         T_REGISTER
            lda         K_CH_OFF1,X
            sty         T_REGISTER
            sta         TA_REQ + RQ_OFFSET + 1
            stz         T_REGISTER
            lda         K_CH_OFF2,X
            sty         T_REGISTER
            sta         TA_REQ + RQ_OFFSET + 2
            stz         T_REGISTER
            lda         K_CH_OFF3,X
            sty         T_REGISTER
            sta         TA_REQ + RQ_OFFSET + 3
            plp
            jmp         f_flags

; The request's offset into channel F_CH
f_setoff:
            ldx         F_CH
            ldy         T_REGISTER
            php
            sei
            lda         TA_REQ + RQ_OFFSET
            stz         T_REGISTER
            sta         K_CH_OFF0,X
            sty         T_REGISTER
            lda         TA_REQ + RQ_OFFSET + 1
            stz         T_REGISTER
            sta         K_CH_OFF1,X
            sty         T_REGISTER
            lda         TA_REQ + RQ_OFFSET + 2
            stz         T_REGISTER
            sta         K_CH_OFF2,X
            sty         T_REGISTER
            lda         TA_REQ + RQ_OFFSET + 3
            stz         T_REGISTER
            sta         K_CH_OFF3,X
            sty         T_REGISTER
            plp
            rts

; A new channel F_CH, from the server's answer: its server, device, fid, mode, qid type; offset 0
f_chset:
            ldx         F_CH
            ldy         T_REGISTER
            php
            sei
            lda         F_SRV
            stz         T_REGISTER                          ; ---- The kernel task's channels
            sta         K_CH_SRV,X
            sty         T_REGISTER
            lda         TA_REQ + RQ_DEV
            stz         T_REGISTER
            sta         K_CH_DEV,X
            sty         T_REGISTER
            lda         TA_REQ + RQ_FID
            stz         T_REGISTER
            sta         K_CH_FID,X
            sty         T_REGISTER
            lda         TA_REQ + RQ_MODE
            stz         T_REGISTER
            sta         K_CH_MODE,X
            sty         T_REGISTER
            lda         TA_REQ + RQ_PERM                    ; (Out: its qid type)
            stz         T_REGISTER
            sta         K_CH_QTYPE,X
            sty         T_REGISTER
            plp
            ldx         #3
:
            stz         TA_REQ + RQ_OFFSET,X
            dex
            bpl         :-
            jmp         f_setoff

; ****************************************************************************
; Names: the current directory, and the namespace (its tables: ns.s, page 3)

; CHDIR: change the current directory.  IN: r0 = its path.  OUT: C = 0; or C = 1, .A = E_NOTDIR, or as OPEN's
K_CHDIR:
            jsr         f_name
            bcs         @done
            jsr         f_probe
            bcs         @done
            and         #QT_DIR
            beq         @notdir
            KCALL_FAR   K_NS_CWD_K                          ; (Its name, whole and clean: the kernel's copy)
@done:
            rts

@notdir:
            FAIL        E_NOTDIR

; GETCWD: the current directory.  IN: r0 = a buffer (PATH_MAX + 1 bytes).  OUT: .A = its length
K_GETCWD:
            ldy         #0
:
            lda         TA_CWD,Y
            sta         (r0),Y
            beq         :+
            iny
            cpy         #PATH_MAX + 1
            bcc         :-
:
            tya
            clc
            rts

; BIND's, MOUNT's and UNMOUNT's failures with one, two or three bytes on the stack: dropped, .A and C kept
f_fail3:
            ply
f_fail2:
            ply
f_fail1:
            ply
            sec
            rts

; BIND: new at old.  IN: r0 = new; r1 = old; .A = the flags: MREPL (in place of what's at old), MBEFORE or MAFTER
; (a union: new first, or last), and MCREATE (a CREATE in the union comes to new).  New is what it is now: a mount
; point itself is all of its union's members, anything else the first of its candidates that's there.  OUT: C = 0;
; or C = 1, .A = E_NOENT (new isn't there), E_INVAL (old isn't absolute), E_NSFULL (the namespace tables are full)
K_BIND:
            pha                                             ; (The flags, and old)
            jsr         f_anchor
            pla
            pha
            lda         r1
            pha
            lda         r1 + 1
            pha
            jsr         f_name                              ; New
            bcs         f_fail3
            ldx         #1                                  ; (.X: the union, or the candidate)
            lda         F_EXACT
            bne         :+
            jsr         f_probe
            bcs         f_fail3
            ldx         #0
:
            pla                                             ; Old
            sta         r0 + 1
            pla
            sta         r0
            phx
            jsr         f_path
            bcs         f_fail2
            plx
            pla
            KCALL_FAR   K_NS_BIND_K
            rts

; MOUNT: a device's server at old.  IN: .X = the device letter; r0 = its spec (8 characters at most), or 0; r1 =
; old; .A = the flags (as BIND's).  OUT: as BIND's, and E_NODEV
K_MOUNT:
            pha
            phx
            jsr         f_anchor
            ldy         #0                                  ; The spec, into TA_SCRATCH, zero-padded
            lda         r0
            ora         r0 + 1
            beq         @pad
:
            lda         (r0),Y
            beq         @pad
            sta         TA_SCRATCH,Y
            iny
            cpy         #8
            bcc         :-
            lda         (r0),Y
            beq         @old
            lda         #E_NAMETOOLONG
            bra         @fail

@pad:
            lda         #0
:
            cpy         #8
            bcs         @old
            sta         TA_SCRATCH,Y
            iny
            bra         :-

@old:
            lda         r1                                  ; Old
            sta         r0
            lda         r1 + 1
            sta         r0 + 1
            jsr         f_path
            bcs         @fail
            plx
            pla
            KCALL_FAR   K_NS_MOUNT_K
            rts

@fail:
            jmp         f_fail2

; UNMOUNT: what's at old gone: all of it (r0 = 0), or new's member there (r0 = new).  IN: r0 = new, or 0; r1 = old.
; OUT: C = 0; or C = 1, .A = E_NOENT (not there), E_INVAL, E_NSFULL
K_UNMOUNT:
            lda         r1                                  ; (Old)
            pha
            lda         r1 + 1
            pha
            ldx         #0                                  ; (.X: all, or the candidate)
            lda         r0
            ora         r0 + 1
            beq         :+
            jsr         f_name
            bcs         @fail2
            jsr         f_probe
            bcs         @fail2
            ldx         #1
:
            pla                                             ; Old
            sta         r0 + 1
            pla
            sta         r0
            phx
            jsr         f_path
            bcs         @fail1
            plx
            KCALL_FAR   K_NS_UNMOUNT_K
            rts

@fail2:
            jmp         f_fail2

@fail1:
            jmp         f_fail1

; Before a union's first bind at old (MBEFORE, MAFTER): what's there now, if it's there and isn't a mount point
; itself, its first member, as in Plan 9 (a union has what was at old).  IN: .A = the flags; r1 = old.  Keeps r0,
; r1 (any error: nothing done)
f_anchor:
            and         #MORDER
            beq         @done
            lda         r0
            pha
            lda         r0 + 1
            pha
            lda         r1
            sta         r0
            lda         r1 + 1
            sta         r0 + 1
            jsr         f_name
            bcs         @back
            lda         F_EXACT
            bne         @back
            jsr         f_probe                             ; (There?)
            bcs         @back
            jsr         f_path                              ; Old again, for the bind
            bcs         @back
            lda         #MREPL
            ldx         #0
            KCALL_FAR   K_NS_BIND_K
@back:
            pla
            sta         r0 + 1
            pla
            sta         r0
@done:
            rts

; The name f_name found: its first candidate that opens (for reading), and the open undone; the candidate stays
; the kernel's (K_CAND: BIND's new).  OUT: C = 0, .A = its qid type; or C = 1, .A = the error (E_NOENT: none)
f_probe:
            lda         #R_OPEN
            sta         TA_REQ + RQ_TYPE
            lda         #O_READ
            sta         TA_REQ + RQ_MODE
            stz         TA_REQ + RQ_PERM
            jsr         f_flags
@cand:
            lda         #0
            jsr         f_cand
            bcs         @done
            jsr         f_zero
            jsr         f_send
            bcc         @found
            jsr         f_pass                              ; (Not in that member, or its device gone: the next)
            bcc         @cand
@done:
            rts

@found:
            lda         TA_REQ + RQ_PERM                    ; (Its qid type)
            pha
            lda         #R_CLUNK
            sta         TA_REQ + RQ_TYPE
            jsr         f_send
            pla
            clc
            rts

; ****************************************************************************
; The servers' calls

; SRV_REGISTER: serve a device letter (its requests come to this task's serve entry).  IN: .A = the letter.
; OUT: C = 0; or C = 1, .A = E_INVAL (not a letter: a space, #, or past ~), E_EXIST (another task's), E_NOMEM
K_SRV_REGISTER:
            KCALL_FAR   K_SRV_REGISTER_K
            rts

; SRV_TAKE: the request this task is serving (its client's TA_REQ) into its TA_INBOX, and for one with a name
; (R_OPEN, R_CREATE, R_REMOVE) the name into its TA_PATH; and this task's event count, as it is before the server
; looks at anything (RQ_EVENT: a client answered E_AGAIN waits for it to change).  At the start of the serve entry.
; OUT: .A = the request (R_*), .Y = the client
K_SRV_TAKE:
            lda         TK_INCALLER                         ; (The client: the task whose call this is)
            sta         K_TASK
            lda         #<TA_INBOX
            sta         K_PTR
            lda         #>TA_INBOX
            sta         K_PTR + 1
            lda         #<TA_REQ
            sta         K_PTR2
            lda         #>TA_REQ
            sta         K_PTR2 + 1
            lda         #RQ_SIZE
            sta         K_CNT
            stz         K_CNT + 1
            lda         K_TASK
            sec
            FARCALL     K_KCOPY
            lda         TK_INCALLER
            sta         TA_INBOX + RQ_CLIENT
            lda         TA_INBOX + RQ_TYPE                  ; A name with it?
            cmp         #R_OPEN
            beq         @name
            cmp         #R_CREATE
            beq         @name
            cmp         #R_REMOVE
            bne         @done
@name:
            lda         #<TA_PATH
            sta         K_PTR
            sta         K_PTR2
            lda         #>TA_PATH
            sta         K_PTR + 1
            sta         K_PTR2 + 1
            ldx         TA_INBOX + RQ_NAMELEN
            cpx         #PATH_MAX + 1
            bcc         :+
            ldx         #PATH_MAX
:
            inx
            stx         K_CNT
            stz         K_CNT + 1
            lda         TA_INBOX + RQ_CLIENT
            sec
            FARCALL     K_KCOPY
            stz         TA_PATH + PATH_MAX
@done:
            lda         TK_EVENT
            sta         TA_INBOX + RQ_EVENT
            lda         TA_INBOX + RQ_TYPE
            ldy         TA_INBOX + RQ_CLIENT
            clc
            rts

; SRV_REPLY: what the request returns (RQ_FID, RQ_DONE, RQ_PERM in TA_INBOX), and RQ_EVENT, back in the client's
; TA_REQ.  At the end of the serve entry.  Keeps .A and C
K_SRV_REPLY:
            php
            pha
            ldx         TA_INBOX + RQ_CLIENT
            ldy         T_REGISTER
            sei
            lda         TA_INBOX + RQ_FID
            QL_PUT      TA_REQ + RQ_FID
            lda         TA_INBOX + RQ_DONE
            QL_PUT      TA_REQ + RQ_DONE
            lda         TA_INBOX + RQ_DONE + 1
            QL_PUT      TA_REQ + RQ_DONE + 1
            lda         TA_INBOX + RQ_PERM
            QL_PUT      TA_REQ + RQ_PERM
            lda         TA_INBOX + RQ_EVENT
            QL_PUT      TA_REQ + RQ_EVENT
            pla
            plp
            rts

; CLIENT_READ: bytes from the client of the request being served.  IN: r0 = here; r1 = there (in the client's
; view); r2 = the count.  OUT: C = 0
K_CLIENT_READ:
            sec
            bra         f_client

; CLIENT_WRITE: bytes to the client.  IN: r0 = here (the data); r1 = there; r2 = the count.  OUT: C = 0
K_CLIENT_WRITE:
            clc
f_client:
            php
            lda         r0
            sta         K_PTR
            lda         r0 + 1
            sta         K_PTR + 1
            lda         r1
            sta         K_PTR2
            lda         r1 + 1
            sta         K_PTR2 + 1
            lda         r2
            sta         K_CNT
            lda         r2 + 1
            sta         K_CNT + 1
            lda         TA_INBOX + RQ_CLIENT
            plp
            FARCALL     K_KCOPY
            clc
            rts

; ****************************************************************************
; In the kernel task (KCALLs; .Y = the caller)

; A free channel, its fd count 1.  OUT: .A = it; or C = 1, .A = E_NFILE
K_CH_NEW_K:
            ldx         #CH_MAX - 1
:
            lda         K_CH_REFS,X
            beq         :+
            dex
            bpl         :-
            FAIL        E_NFILE
:
            lda         #1
            sta         K_CH_REFS,X
            lda         #$FF
            sta         K_CH_SRV,X
            sta         K_CH_UFROM,X                        ; (Not a union directory)
            txa                                             ; Its name: the caller's being resolved (K_RES: the
            jsr         f_chname                            ;   name OPEN or CREATE was given, whole and clean)
            tya
            lsr
            lsr
            clc
            adc         #>K_RES
            sta         K_PTR2 + 1
            tya
            and         #3
            lsr
            ror
            ror
            sta         K_PTR2                              ; (<K_RES = 0)
            ldy         #0                                  ; (To its 0: SPAWN makes a channel, and every cycle
:                                                           ;   counts there)
            lda         (K_PTR2),Y
            sta         (K_PTR),Y
            beq         :+
            iny
            cpy         #PATH_MAX + 1
            bcc         :-
:
            txa
            clc
            rts

; K_PTR = channel .A's name (K_CH_NAME + 64 * it), in the kernel task.  Keeps .X, .Y
f_chname:
            pha
            lsr
            lsr
            clc
            adc         #>K_CH_NAME
            sta         K_PTR + 1
            pla
            and         #3
            lsr
            ror
            ror
            sta         K_PTR                               ; (<K_CH_NAME = 0)
            rts

.assert     <K_RES = 0, error, "K_CH_NEW_K: K_RES is page-aligned"

; Device letter .A's server.  OUT: .A = its task; or C = 1, .A = E_NODEV
K_DEV_FIND_K:
            ldx         #DEV_MAX - 1
:
            cmp         K_DEV_LETTER,X
            beq         :+
            dex
            bpl         :-
            FAIL        E_NODEV
:
            lda         K_DEV_TASK,X
            clc
            rts

; Letter .A for task .Y (SRV_REGISTER)
K_SRV_REGISTER_K:
            sty         K0_TMP                              ; (The caller)
            cmp         #'!'                                ; (Printable, not #)
            bcc         @inval
            cmp         #'~' + 1
            bcs         @inval
            cmp         #'#'
            beq         @inval
            ldx         #DEV_MAX - 1                        ; Already someone's?
:
            cmp         K_DEV_LETTER,X
            beq         @taken
            dex
            bpl         :-
            ldx         #DEV_MAX - 1                        ; A free entry
:
            ldy         K_DEV_LETTER,X
            beq         @free
            dex
            bpl         :-
            FAIL        E_NOMEM

@taken:
            lda         K0_TMP
            cmp         K_DEV_TASK,X
            bne         @exist
            clc                                             ; (Its own already)
            rts

@free:
            sta         K_DEV_LETTER,X
            lda         K0_TMP
            sta         K_DEV_TASK,X
            clc
            rts

@inval:
            FAIL        E_INVAL

@exist:
            FAIL        E_EXIST

; Task .Y has ended: its device letters free, the channels it served dead (FARCALL from K_EXIT_K).  Keeps .Y
K_FILE_EXIT:
            ldx         #DEV_MAX - 1
@dev:
            tya
            cmp         K_DEV_TASK,X
            bne         :+
            lda         K_DEV_LETTER,X
            beq         :+
            stz         K_DEV_LETTER,X
:
            dex
            bpl         @dev
            ldx         #CH_MAX - 1
@ch:
            lda         K_CH_REFS,X
            beq         :+
            tya
            cmp         K_CH_SRV,X
            bne         :+
            lda         #$FF
            sta         K_CH_SRV,X
:
            dex
            bpl         @ch
            rts

; A child's fds: the parent's that SPAWN's map names (the parent's TA_SCRATCH + SP_MAP: its fd for each of the
; child's, $FF for none), the channels' fds counted (FARCALL from K_SPAWN_K: .X = the child, K0_TMP3 = the parent)
K_FD_INHERIT:
            stx         K0_TMP
            ldy         #FD_MAX - 1
@fd:
            ldx         K0_TMP3                             ; The parent's fd for it, and that fd's channel ...
            php
            sei
            stx         T_REGISTER
            lda         TA_SCRATCH + SP_MAP,Y
            cmp         #FD_MAX
            bcc         :+
            lda         #$FF                                ; (None: no channel)
            bra         :++
:
            tax
            lda         TA_FD,X
:
            stz         T_REGISTER
            plp
            cmp         #CH_MAX
            bcs         @next
            tax
            inc         K_CH_REFS,X
            ldx         K0_TMP                              ; ... the child's
            php
            sei
            stx         T_REGISTER
            sta         TA_FD,Y
            stz         T_REGISTER
            plp
@next:
            dey
            bpl         @fd
            ldx         K0_TMP
            rts

.segment "KRODATA_P2"
f_s_pipe:   .byte       "#|/pipe", 0
