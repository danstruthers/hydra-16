; ****************************************************************************
; kdev - the kernel's own devices (docs/reimplementation-from-scratch.md, §14.1), served by a driver of their own
; on srvlib (a boot driver), not by the kernel task: it keeps only tables.  One tree each:
;   #/      the root: an empty directory for each mount point of the default namespace (bin dev env lib mnt pc
;           proc ram rom sd sram tmp, and in dev: gpio i2c mod sd spi), so ls / and ls /dev show them
;   #n      null (reads as nothing, takes every write), zero (reads as zeros, takes every write)
;   #t      ticks: the tick count (its low 16 bits, TICK_HZ a second), in decimal
;   #m      the modules in the paged ROM, a file each: its image (its header first: SPAWN reads it); bin, the
;           programs alone (bound at /bin)
;   #p      the tasks, a directory each (its number): status (its name, state, parent, CPU time in ticks and note
;           group) and ctl (kill, interrupt, note N)
;   #|      pipes: opening pipe makes a new one (its read end; for O_WRITE, its write end), and R_DUP its other end
;           (PIPE does both); 512 bytes each, 8 of them
;   #e      the environment of the task asking (the kernel keeps it: ENV_GET ...), a file a variable
; To come: /proc's other files (args, cwd, fd, ns ...).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"

            HYX2_DRIVER "kdev", init, srv_serve, 0, 0, HF_BOOT

SRV_STAT        = mod_stat                                  ; (srvlib: a module's file's length)

PIPE_N          = 8
PIPE_SIZE       = 512
E_FIDS          = 16                                        ; #e's fids ...
EF_DIR          = 1                                         ;   each the directory ...
EF_VAR          = 2                                         ;   or a variable (0: free)

.zeropage
pp:         .res        1                                   ; A pipe ...
pend:       .res        1                                   ;   and its end ($80: the write end)
n:          .res        2                                   ; A count
m:          .res        2                                   ; Another
pt:         .res        2                                   ; A pointer
tk:         .res        1                                   ; A task
cnt:        .res        1
want:       .res        1                                   ; A module type wanted (0: any)
left:       .res        1

.bss
me:         .res        ME_SIZE                             ; A module (MODINFO)
last_want:  .res        1                                   ; The last module h_list named: of this type ($FF:
last_k:     .res        1                                   ;   none yet), the k-th ...
last_cnt:   .res        1                                   ;   at this entry
chunk:      .res        256                                 ; A page of one (ROMREAD)
info:       .res        TI_SIZE                             ; A task (TASKINFO)
p_used:     .res        PIPE_N                              ; Each pipe: in use ...
p_rdl:      .res        PIPE_N                              ;   where the next read is ...
p_rdh:      .res        PIPE_N
p_wrl:      .res        PIPE_N                              ;   the next write ...
p_wrh:      .res        PIPE_N
p_cntl:     .res        PIPE_N                              ;   the bytes in it ...
p_cnth:     .res        PIPE_N
p_readers:  .res        PIPE_N                              ;   its ends' fids
p_writers:  .res        PIPE_N
p_buf:      .res        PIPE_N * PIPE_SIZE
ef_kind:    .res        E_FIDS                              ; #e's fids: each its kind (EF_*) ...
ef_task:    .res        E_FIDS                              ;   the task whose environment it's in ...
ef_name:    .res        E_FIDS * (ENV_NAME_MAX + 1)         ;   and a variable's name
ename:      .res        ENV_NAME_MAX + 1                    ; A request's name

.code
; ****************************************************************************
; Init: every device's letter
init:
            ldx         #PIPE_N - 1
:
            stz         p_used,X
            dex
            bpl         :-
            lda         #$FF
            sta         last_want
            ldx         #0
@letter:
            lda         SRV_TREES,X
            beq         @done
            phx
            jsr         SRV_REGISTER
            plx
            bcs         @failed
            inx
            inx
            inx
            bra         @letter

@done:
            clc
@failed:
            rts

; ****************************************************************************
; #n

h_null:
            cmp         #R_READ
            bne         h_take
            stz         TASK_INBOX + RQ_DONE                ; (Nothing: the end)
            stz         TASK_INBOX + RQ_DONE + 1
            clc
            rts

; A write: all of it taken.  Opens and clunks: nothing to do
h_take:
            cmp         #R_WRITE
            bne         :+
            MOVR        TASK_INBOX + RQ_DONE, TASK_INBOX + RQ_COUNT
:
            clc
            rts

h_zero:
            cmp         #R_READ
            bne         h_take
            MOVR        n, TASK_INBOX + RQ_COUNT            ; Zeros, 64 at a time
            MOVR        r1, TASK_INBOX + RQ_BUF
@part:
            lda         n
            ora         n + 1
            beq         @done
            lda         n + 1
            bne         @full
            lda         n
            cmp         #64
            bcc         :+
@full:
            lda         #64
:
            sta         r2
            stz         r2 + 1
            LDR         r0, zeros
            jsr         CLIENT_WRITE
            sec
            lda         n
            sbc         r2
            sta         n
            bcs         :+
            dec         n + 1
:
            clc
            lda         r1
            adc         r2
            sta         r1
            bcc         @part
            inc         r1 + 1
            bra         @part

@done:
            MOVR        TASK_INBOX + RQ_DONE, TASK_INBOX + RQ_COUNT
            clc
            rts

; ****************************************************************************
; #t

gen_ticks:
            jsr         TICKS
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; ****************************************************************************
; #m: the modules, a file each (its id: its entry in the module directory), read as its image in the paged ROM, its
; header first (SPAWN reads it so: a module runs in place); #m/bin, the programs alone (bind -a '#m/bin' /bin)

h_bins:
            ldy         #HT_PROGRAM                         ; (#m/bin: the programs)
            bra         h_list

h_mods:
            ldy         #0                                  ; (#m: every module)
h_list:
            sty         want
            cmp         #DYN_FIND
            beq         @find
            cmp         #DYN_IDNAME
            beq         @idname
            lda         z:srv_k                             ; DYN_NAME: the srv_k-th of those wanted
            sta         left
            stz         cnt
            lda         want                                ; (The one after the last named: on from it, not
            cmp         last_want                           ;   from the start again, as a directory is read)
            bne         @name
            lda         last_k
            inc         a
            cmp         z:srv_k
            bne         @name
            ldx         last_cnt
            inx
            stx         cnt
            stz         left
@name:
            jsr         mod_next
            bcs         @done
            lda         left
            beq         @named
            dec         left
            inc         cnt
            bra         @name

@named:
            lda         want
            sta         last_want
            lda         z:srv_k
            sta         last_k
            lda         cnt
            sta         last_cnt
@this:
            lda         cnt
            clc
@done:
            rts

@idname:
            txa
            jmp         mod_name

@find:                                                      ; The one named at srv_p
            stz         cnt
@try:
            jsr         mod_next
            bcs         @noent
            LDR         r3, srv_dname
            jsr         srv_same
            beq         @this
            inc         cnt
            bra         @try

@noent:
            lda         #E_NOENT
            sec
            rts

; The first module wanted (want: a type, or 0 for any) from entry cnt on: cnt = it, its entry in me, its name in
; srv_dname.  OUT: C = 0; or C = 1: no more
mod_next:
            lda         cnt
            jsr         mod_name
            bcs         @done
            lda         want
            beq         @done                               ; (C = 0)
            cmp         me + ME_TYPE
            clc
            beq         @done
            inc         cnt
            bra         mod_next

@done:
            rts

; Module .A: its entry in me, its name in srv_dname.  OUT: C = 0; or C = 1: no such module
mod_name:
            pha
            LDR         r0, me
            pla
            jsr         MODINFO
            bcs         @done
            ldx         #0
:
            lda         me + ME_NAME,X
            sta         srv_dname,X
            beq         :+
            inx
            cpx         #12
            bne         :-
            stz         srv_dname,X
:
            clc
@done:
            rts

; Module .A: its entry in me, and its image's length in n (from its header: its last bank's, after 16K for each bank
; before it).  OUT: C = 0; or C = 1, .A = E_NOENT (no such module)
mod_length:
            jsr         mod_name
            bcs         @done
            LDR         r0, PROM_WINDOW + HX_LENGTH
            LDR         r1, n
            LDR         r2, 2
            lda         me + ME_BANK
            jsr         ROMREAD
            bcs         @done
            lda         me + ME_BANKS
            dec         a
            .repeat     6                                   ; (Banks before the last: 64 pages each)
            asl
            .endrepeat
            clc
            adc         n + 1
            sta         n + 1
            clc
@done:
            rts

; A module's file: from the read's offset, a page of the paged ROM at a time (through chunk: ROMREAD) to the client.
; Opens and clunks: nothing to do
h_image:
            cmp         #R_READ
            beq         :+
            clc
            rts
:
            lda         srv_fid_aux,X
            jsr         mod_length                          ; n: its length
            bcs         @done
            jsr         img_left                            ; m: what to send
            MOVR        pt, TASK_INBOX + RQ_OFFSET          ; pt: where in the image
@part:
            lda         m
            ora         m + 1
            beq         @end
            jsr         img_part
            bcc         @part
            rts

@end:
            clc
@done:
            rts

; m = what's left of the image (n bytes) after the read's offset (none past its end), or what the read asks for if
; less; RQ_DONE = 0
img_left:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            stz         m
            stz         m + 1
            lda         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @done
            sec
            lda         n
            sbc         TASK_INBOX + RQ_OFFSET
            tax
            lda         n + 1
            sbc         TASK_INBOX + RQ_OFFSET + 1
            bcc         @done
            stx         m
            sta         m + 1
            lda         TASK_INBOX + RQ_COUNT
            cmp         m
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         m + 1
            bcs         @done
            MOVR        m, TASK_INBOX + RQ_COUNT
@done:
            rts

; The next part, to pt's page's end (or m bytes, if less), from the paged ROM to the client, after what's sent;
; pt, m and RQ_DONE moved on.  OUT: C = 0; or C = 1, .A = ROMREAD's error
img_part:
            lda         #0                                  ; r2: to the page's end (256 - pt's low byte) ...
            sec
            sbc         pt
            sta         r2
            lda         #1
            sbc         #0
            sta         r2 + 1
            lda         m                                   ;   or what's left, if less
            cmp         r2
            lda         m + 1
            sbc         r2 + 1
            bcs         :+
            MOVR        r2, m
:
            lda         pt                                  ; r0: where in its bank (pt's bank: pt / 16K after
            sta         r0                                  ;   its first)
            lda         pt + 1
            and         #$3F
            ora         #>PROM_WINDOW
            sta         r0 + 1
            LDR         r1, chunk
            lda         pt + 1
            rol
            rol
            rol
            and         #3
            clc
            adc         me + ME_BANK
            jsr         ROMREAD
            bcs         @done
            LDR         r0, chunk                           ; To the client: its buffer, after what's sent
            clc
            lda         TASK_INBOX + RQ_BUF
            adc         TASK_INBOX + RQ_DONE
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            adc         TASK_INBOX + RQ_DONE + 1
            sta         r1 + 1
            jsr         CLIENT_WRITE
            clc
            lda         TASK_INBOX + RQ_DONE
            adc         r2
            sta         TASK_INBOX + RQ_DONE
            lda         TASK_INBOX + RQ_DONE + 1
            adc         r2 + 1
            sta         TASK_INBOX + RQ_DONE + 1
            clc
            lda         pt
            adc         r2
            sta         pt
            lda         pt + 1
            adc         r2 + 1
            sta         pt + 1
            sec
            lda         m
            sbc         r2
            sta         m
            lda         m + 1
            sbc         r2 + 1
            sta         m + 1
            clc
@done:
            rts

; srvlib's SRV_STAT: a module's file's length, its image's
mod_stat:
            ldy         #SE_HANDLER                         ; (A module's file: h_image's)
            lda         (srv_ent),Y
            cmp         #<h_image
            bne         @done
            iny
            lda         (srv_ent),Y
            cmp         #>h_image
            bne         @done
            lda         z:srv_id
            jsr         mod_length
            bcs         @done
            MOVR        srv_stat + SR_LENGTH, n
@done:
            rts

; ****************************************************************************
; #e: the environment of the task asking (the kernel's: ENV_GET, ENV_PUT, ENV_DEL, ENV_NAME), a file a variable.  A
; raw device (SK_RAW: its fids its own, E_FIDS of them): a fid is the directory, or a variable (its name, and the
; task whose it is: the opener's, so a child given the fd reads its parent's).  Opening one with O_TRUNC, or
; creating one, empties it (or makes it); a write sets the value from its offset to its end (rc writes a variable
; whole, from 0); a read past the end is the end.  The directory reads as a stat record a variable, in the order
; they were made.

h_env:
            ldx         #E_NREQ - 1
:
            cmp         e_reqs,X
            beq         :+
            dex
            bpl         :-
            lda         #E_NOSYS
            sec
            rts
:
            txa
            asl
            tax
            jmp         (e_reqvec,X)

; R_OPEN: the directory (for reading), or a variable there is
e_open:
            jsr         e_path
            bcs         e_done
            beq         @dir
            jsr         e_length                            ; (There?)
            bcs         e_done
            lda         TASK_INBOX + RQ_MODE
            and         #O_TRUNC
            beq         :+
            jsr         e_empty
            bcs         e_done
:
            lda         #EF_VAR
            jmp         e_newfid

@dir:
            lda         TASK_INBOX + RQ_MODE
            and         #O_RW_MASK | O_TRUNC
            beq         :+
            lda         #E_ISDIR
            sec
            rts
:
            lda         #EF_DIR
            jmp         e_newfid

; R_CREATE: a variable, empty (made, or emptied)
e_create:
            jsr         e_path
            bcs         e_done
            beq         @perm
            lda         TASK_INBOX + RQ_PERM                ; (No directories)
            and         #DM_DIR
            bne         @perm
            jsr         e_empty
            bcs         e_done
            lda         #EF_VAR
            jmp         e_newfid

@perm:
            lda         #E_PERM
            sec
e_done:
            rts

; R_REMOVE: a variable
e_remove:
            jsr         e_path
            bcs         e_done
            beq         @perm
            LDR         r0, ename
            lda         TASK_INBOX + RQ_CLIENT
            jmp         ENV_DEL

@perm:
            lda         #E_PERM
            sec
            rts

; R_CLUNK
e_clunk:
            jsr         e_fid
            bcs         e_done
            stz         ef_kind,X
e_ok:
            clc
            rts

; R_DUP: another fid as the request's
e_dup:
            jsr         e_fid
            bcs         e_done
            stx         cnt
            jsr         e_slot                              ; (.X: a free one)
            bcs         e_done
            ldy         cnt
            lda         ef_kind,Y
            sta         ef_kind,X
            lda         ef_task,Y
            sta         ef_task,X
            lda         cnt                                 ; Its name: the old one's
            jsr         e_namep
            MOVR        pt, r0
            txa
            jsr         e_namep
            ldy         #ENV_NAME_MAX
:
            lda         (pt),Y
            sta         (r0),Y
            dey
            bpl         :-
            jmp         e_answer

; R_STAT: the directory's record, or the variable's
e_stat:
            jsr         e_fid
            bcs         e_done
            jsr         e_record
            bcs         e_done
            LDR         r0, srv_stat
            MOVR        r1, TASK_INBOX + RQ_BUF
            LDR         r2, SR_SIZE
            MOVR        TASK_INBOX + RQ_DONE, r2
            jsr         CLIENT_WRITE
            clc
            rts

; R_READ: the directory's records, or the value from the offset (256 bytes at a time, through chunk)
e_read:
            jsr         e_fid
            bcs         @done
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         ef_kind,X
            cmp         #EF_DIR
            bne         @value
            jmp         e_readdir

@value:
            lda         TASK_INBOX + RQ_OFFSET + 2          ; (Past 64K: past the end)
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @end
@part:
            jsr         e_part                              ; n: what's left of the count, 256 at most
            beq         @end
            ldx         TASK_INBOX + RQ_FID
            jsr         e_at                                ; r0: the name; r3: the offset + what's done
            LDR         r1, chunk
            MOVR        r2, n
            lda         ef_task,X
            jsr         ENV_GET
            bcs         @done
            sta         m                                   ; n: what came (the value's rest, n at most)
            stx         m + 1
            lda         m
            cmp         n
            lda         m + 1
            sbc         n + 1
            bcs         :+
            MOVR        n, m
:
            lda         n
            ora         n + 1
            beq         @end
            LDR         r0, chunk                           ; To the client, after what's sent
            jsr         e_client
            MOVR        r2, n
            jsr         CLIENT_WRITE
            jsr         e_moved
            lda         n + 1                               ; (A whole 256: there may be more)
            bne         @part
@end:
            clc
@done:
            rts

; R_WRITE: the value from the offset, 256 bytes at a time (through chunk)
e_write:
            jsr         e_fid
            bcs         @done
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         ef_kind,X
            cmp         #EF_DIR
            beq         @isdir
            lda         TASK_INBOX + RQ_OFFSET + 2          ; (Past 64K: no room)
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @nomem
@part:
            jsr         e_part
            beq         @end
            LDR         r0, chunk                           ; From the client ...
            jsr         e_client
            MOVR        r2, n
            jsr         CLIENT_READ
            ldx         TASK_INBOX + RQ_FID                 ; ... into the value
            jsr         e_at
            LDR         r1, chunk
            MOVR        r2, n
            lda         ef_task,X
            jsr         ENV_PUT
            bcs         @done
            jsr         e_moved
            bra         @part

@end:
            clc
@done:
            rts

@isdir:
            lda         #E_ISDIR
            sec
            rts

@nomem:
            lda         #E_NOMEM
            sec
            rts

; The directory's records from the offset (a record's start), as many as the count holds: a variable each
e_readdir:
            lda         TASK_INBOX + RQ_OFFSET              ; (A record's start)
            and         #SR_SIZE - 1
            bne         @inval
            lda         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @end
            lda         TASK_INBOX + RQ_OFFSET + 1          ; cnt: the first record's number (offset / 64)
            sta         cnt
            lda         TASK_INBOX + RQ_OFFSET
            asl
            rol         cnt
            asl
            rol         cnt
@record:
            sec                                             ; Room for another?
            lda         TASK_INBOX + RQ_COUNT
            sbc         TASK_INBOX + RQ_DONE
            tay
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         TASK_INBOX + RQ_DONE + 1
            bne         :+
            cpy         #SR_SIZE
            bcc         @end
:
            LDR         r0, ename
            ldx         TASK_INBOX + RQ_FID
            lda         ef_task,X
            ldx         cnt
            jsr         ENV_NAME                            ; (Past the last: the end)
            bcs         @end
            jsr         e_varrec
            LDR         r0, srv_stat
            jsr         e_client
            LDR         r2, SR_SIZE
            jsr         CLIENT_WRITE
            clc
            lda         TASK_INBOX + RQ_DONE
            adc         #SR_SIZE
            sta         TASK_INBOX + RQ_DONE
            bcc         :+
            inc         TASK_INBOX + RQ_DONE + 1
:
            inc         cnt
            bra         @record

@end:
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; ---- #e's pieces

; The request's name (TASK_PATH, past its slashes) into ename.  OUT: C = 0, Z = 1: none (the directory); Z = 0: a
; variable's; or C = 1, .A = E_NOENT (a name in a variable), E_NAMETOOLONG
e_path:
            ldx         #0
:
            lda         TASK_PATH,X
            cmp         #'/'
            bne         :+
            inx
            bra         :-
:
            ldy         #0
@char:
            lda         TASK_PATH,X
            sta         ename,Y
            beq         @end
            cmp         #'/'
            beq         @noent
            inx
            iny
            cpy         #ENV_NAME_MAX + 1
            bne         @char
            lda         #E_NAMETOOLONG
            sec
            rts

@end:
            cpy         #0                                  ; (Z: none)
            clc
            rts

@noent:
            lda         #E_NOENT
            sec
            rts

; The client's variable ename: its length in n.  OUT: C = 0; or C = 1, .A = E_NOENT
e_length:
            LDR         r0, ename
            stz         r2
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            lda         TASK_INBOX + RQ_CLIENT
            jsr         ENV_GET
            sta         n
            stx         n + 1
            rts

; The client's variable ename, empty (made, if it isn't there).  OUT: C = 0; or C = 1, .A = ENV_PUT's error
e_empty:
            LDR         r0, ename
            stz         r2
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            lda         TASK_INBOX + RQ_CLIENT
            jmp         ENV_PUT

; A fid made, of kind .A, for the client and ename; the answer: RQ_FID, RQ_PERM.  OUT: C = 0; or C = 1, .A = E_NFILE
e_newfid:
            pha
            jsr         e_slot
            pla
            bcc         :+
            rts
:
            sta         ef_kind,X
            lda         TASK_INBOX + RQ_CLIENT
            sta         ef_task,X
            txa
            jsr         e_namep
            ldy         #ENV_NAME_MAX
:
            lda         ename,Y
            sta         (r0),Y
            dey
            bpl         :-
e_answer:                                                   ; (Fid .X: the answer)
            stx         TASK_INBOX + RQ_FID
            lda         ef_kind,X
            cmp         #EF_DIR
            lda         #QT_FILE
            bcc         :+
            lda         #QT_DIR
:
            sta         TASK_INBOX + RQ_PERM
            clc
            rts

; A free fid.  OUT: C = 0, .X = it; or C = 1, .A = E_NFILE
e_slot:
            ldx         #E_FIDS - 1
:
            lda         ef_kind,X
            beq         @got
            dex
            bpl         :-
            lda         #E_NFILE
            sec
            rts

@got:
            clc
            rts

; The request's fid: .X.  OUT: C = 0; or C = 1, .A = E_BADF
e_fid:
            ldx         TASK_INBOX + RQ_FID
            cpx         #E_FIDS
            bcs         @badf
            lda         ef_kind,X
            beq         @badf
            clc
            rts

@badf:
            lda         #E_BADF
            sec
            rts

; r0 = fid .A's name (ef_name + .A * 32).  Keeps .X
e_namep:
            stz         r0 + 1
            .repeat     5
            asl
            rol         r0 + 1
            .endrepeat
            clc
            adc         #<ef_name
            sta         r0
            lda         r0 + 1
            adc         #>ef_name
            sta         r0 + 1
            rts

.assert     ENV_NAME_MAX + 1 = 32, error, "e_namep: a fid's name is 32 bytes"

; r0 = fid .X's name; r3 = the request's offset + what's done.  Keeps .X
e_at:
            txa
            jsr         e_namep
            clc
            lda         TASK_INBOX + RQ_OFFSET
            adc         TASK_INBOX + RQ_DONE
            sta         r3
            lda         TASK_INBOX + RQ_OFFSET + 1
            adc         TASK_INBOX + RQ_DONE + 1
            sta         r3 + 1
            rts

; n = what's left of the request's count (256 at most).  OUT: Z = 1: none
e_part:
            sec
            lda         TASK_INBOX + RQ_COUNT
            sbc         TASK_INBOX + RQ_DONE
            sta         n
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         TASK_INBOX + RQ_DONE + 1
            sta         n + 1
            beq         :+
            lda         #1                                  ; (256)
            sta         n + 1
            stz         n
:
            lda         n
            ora         n + 1
            rts

; r1 = the client's buffer, after what's done
e_client:
            clc
            lda         TASK_INBOX + RQ_BUF
            adc         TASK_INBOX + RQ_DONE
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            adc         TASK_INBOX + RQ_DONE + 1
            sta         r1 + 1
            rts

; RQ_DONE += n
e_moved:
            clc
            lda         TASK_INBOX + RQ_DONE
            adc         n
            sta         TASK_INBOX + RQ_DONE
            lda         TASK_INBOX + RQ_DONE + 1
            adc         n + 1
            sta         TASK_INBOX + RQ_DONE + 1
            rts

; srv_stat: fid .X's record (the directory's, or its variable's: its length now).  OUT: C = 0; or C = 1, .A = ENV_GET's
; error (the variable gone)
e_record:
            lda         ef_kind,X
            cmp         #EF_DIR
            beq         @dir
            phx
            txa
            jsr         e_namep
            ldy         #ENV_NAME_MAX                       ; Its name, into ename
:
            lda         (r0),Y
            sta         ename,Y
            dey
            bpl         :-
            plx
            LDR         r0, ename
            stz         r2
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            lda         ef_task,X
            jsr         ENV_GET
            bcc         e_varrec                            ; (.A/.X: its length)
            rts

@dir:
            jsr         e_blank
            lda         #'/'
            sta         srv_stat + SR_NAME
            lda         #QT_DIR
            sta         srv_stat + SR_QTYPE
            lda         #$6D                                ; (r-x for all, a directory)
            sta         srv_stat + SR_MODE
            lda         #$01 | DM_DIR
            sta         srv_stat + SR_MODE + 1
            clc
            rts

; srv_stat: a variable's record (ename, .A/.X its length).  OUT: C = 0
e_varrec:
            pha
            phx
            jsr         e_blank
            ldx         #ENV_NAME_MAX
:
            lda         ename,X
            sta         srv_stat + SR_NAME,X
            dex
            bpl         :-
            lda         #$B6                                ; (rw for all)
            sta         srv_stat + SR_MODE
            lda         #$01
            sta         srv_stat + SR_MODE + 1
            pla
            sta         srv_stat + SR_LENGTH + 1
            pla
            sta         srv_stat + SR_LENGTH
            clc
            rts

; srv_stat empty, but for its device
e_blank:
            ldx         #SR_SIZE - 1
:
            stz         srv_stat,X
            dex
            bpl         :-
            lda         #'e'
            sta         srv_stat + SR_DEV
            rts

; ****************************************************************************
; #p: the tasks, a directory each (its id: the task)

h_procs:
            cmp         #DYN_FIND
            beq         @find
            cmp         #DYN_IDNAME
            beq         @idname
            stz         tk                                  ; DYN_NAME: the srv_k-th task in use
            ldx         z:srv_k
            stx         cnt
@task:
            lda         tk
            cmp         #16
            bcs         @none
            jsr         in_use
            bcs         @next
            lda         cnt
            beq         @this
            dec         cnt
@next:
            inc         tk
            bra         @task

@this:
            ldx         tk
            jsr         task_name
            lda         tk
            clc
            rts

@none:
            sec
            rts

@idname:
            jsr         task_name
            clc
            rts

@find:                                                      ; The number at srv_p, a task in use
            lda         z:srv_p
            sta         pt
            lda         z:srv_p + 1
            sta         pt + 1
            stz         tk
            ldy         #0
@digit:
            lda         (pt),Y
            beq         @end
            cmp         #'/'
            beq         @end
            sec
            sbc         #'0'
            cmp         #10
            bcs         @noent
            sta         cnt
            lda         tk                                  ; * 10, + the digit
            asl
            asl
            clc
            adc         tk
            asl
            clc
            adc         cnt
            sta         tk
            iny
            cpy         #3
            bcc         @digit
            bra         @noent

@end:
            tya
            beq         @noent
            lda         tk
            cmp         #16
            bcs         @noent
            jsr         in_use
            bcs         @noent
            lda         tk
            clc
            rts

@noent:
            lda         #E_NOENT
            sec
            rts

; C = 0 if task .A is in use (its TASKINFO in info).  Keeps tk
in_use:
            pha
            LDR         r0, info
            pla
            jsr         TASKINFO
            bcs         @done
            lda         info + TI_STATE                     ; (0: free)
            beq         @free
            clc
@done:
            rts

@free:
            sec
            rts

; Task .X's name in /proc: its number, in decimal
task_name:
            ldy         #0
            txa
            cmp         #10
            bcc         :+
            pha
            lda         #'1'
            sta         srv_dname
            pla
            sbc         #10                                 ; (C = 1)
            iny
:
            ora         #'0'
            sta         srv_dname,Y
            lda         #0
            sta         srv_dname + 1,Y
            rts

; status: "NAME STATE PARENT CPU GROUP" (the parent - for none; the CPU time in ticks, its low 16 bits)
gen_status:
            lda         z:srv_id
            jsr         in_use
            bcs         @gone
            lda         #<(info + TI_NAME)
            ldx         #>(info + TI_NAME)
            jsr         srv_tputs
            jsr         space
            lda         info + TI_STATE
            cmp         #STATES
            bcc         :+
            lda         #0
:
            asl
            tax
            lda         state_words,X
            pha
            lda         state_words + 1,X
            tax
            pla
            jsr         srv_tputs
            jsr         space
            lda         info + TI_PARENT
            bpl         :+
            lda         #'-'
            jsr         srv_tputc
            bra         :++
:
            ldx         #0
            jsr         srv_tputdec
:
            jsr         space
            lda         info + TI_CPU
            ldx         info + TI_CPU + 1
            jsr         srv_tputdec
            jsr         space
            lda         info + TI_GROUP
            ldx         #0
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
            rts

@gone:
            lda         #E_SRCH
            sec
            rts

space:
            lda         #' '
            jmp         srv_tputc

; ctl: kill, interrupt, note N
c_kill:
            ldx         #NOTE_KILL
            bra         c_post

c_intr:
            ldx         #NOTE_INTERRUPT
            bra         c_post

c_note:
            lda         z:srv_argn
            beq         @inval
            lda         srv_arg + 1
            bne         @inval
            ldx         srv_arg
            bra         c_post

@inval:
            lda         #E_INVAL
            sec
            rts

c_post:
            lda         z:srv_id
            jmp         NOTE_POST

; ****************************************************************************
; #|: pipes (a fid's aux: its pipe, and $80 for the write end)

h_pipe:
            cmp         #R_OPEN
            beq         p_open
            cmp         #R_DUP
            beq         p_dup
            pha
            lda         srv_fid_aux,X
            and         #$80
            sta         pend
            lda         srv_fid_aux,X
            and         #$7F
            sta         pp
            pla
            cmp         #R_READ
            bne         :+
            jmp         p_read
:
            cmp         #R_WRITE
            bne         :+
            jmp         p_write
:
            cmp         #R_CLUNK
            beq         p_clunk
            clc
            rts

; A new pipe: its read end, or (O_WRITE) its write end
p_open:
            ldy         #PIPE_N - 1
:
            lda         p_used,Y
            beq         :+
            dey
            bpl         :-
            lda         #E_NOMEM
            sec
            rts
:
            sty         pp
            lda         #1
            sta         p_used,Y
            lda         #0
            sta         p_rdl,Y
            sta         p_rdh,Y
            sta         p_wrl,Y
            sta         p_wrh,Y
            sta         p_cntl,Y
            sta         p_cnth,Y
            sta         p_readers,Y
            sta         p_writers,Y
            lda         TASK_INBOX + RQ_MODE                ; Its end
            and         #O_RW_MASK
            cmp         #O_WRITE
            bne         :+
            lda         #$80
:
            and         #$80
            bra         p_end

; R_DUP: the other end of fid .Y's pipe
p_dup:
            lda         srv_fid_aux,Y
            and         #$7F
            sta         pp
            lda         srv_fid_aux,Y
            eor         #$80
            and         #$80
; Fid .X: pipe pp's end .A (counted)
p_end:
            sta         pend
            ora         pp
            sta         srv_fid_aux,X
            ldy         pp
            lda         pend
            bne         :+
            lda         p_readers,Y
            inc         a
            sta         p_readers,Y
            clc
            rts
:
            lda         p_writers,Y
            inc         a
            sta         p_writers,Y
            clc
            rts

; An end closed: the pipe's free once both are; the other end's waiters look again
p_clunk:
            ldy         pp
            lda         pend
            bne         :+
            lda         p_readers,Y
            beq         @done
            dec         a
            sta         p_readers,Y
            bra         @done
:
            lda         p_writers,Y
            beq         @done
            dec         a
            sta         p_writers,Y
@done:
            lda         p_readers,Y
            ora         p_writers,Y
            bne         :+
            lda         #0
            sta         p_used,Y
:
            inc         TASK_EVENT
            clc
            rts

; The pipes' failures
p_broken:
            lda         #E_PIPE
            sec
            rts

p_again:
            lda         #E_AGAIN
            sec
            rts

p_badf:
            lda         #E_BADF
            sec
            rts

; A read: what's there, as far as the buffer's end; nothing and no writer left: the end; nothing: E_AGAIN
p_read:
            lda         pend
            bne         p_badf
            ldy         pp
            lda         p_cntl,Y
            ora         p_cnth,Y
            bne         @some
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         p_writers,Y
            bne         p_again
            clc                                             ; (No writer: the end)
            rts

@some:
            lda         p_cntl,Y                            ; m: what's there ...
            sta         m
            lda         p_cnth,Y
            sta         m + 1
            sec                                             ; n: as far as the buffer's end
            lda         #<PIPE_SIZE
            sbc         p_rdl,Y
            sta         n
            lda         #>PIPE_SIZE
            sbc         p_rdh,Y
            sta         n + 1
            jsr         least                               ; n: the least of them and the count
            lda         p_rdl,Y                             ; From the read place
            ldx         p_rdh,Y
            jsr         p_at
            MOVR        r1, TASK_INBOX + RQ_BUF
            MOVR        r2, n
            jsr         CLIENT_WRITE
            ldy         pp
            clc                                             ; The read place on (round), the count down
            lda         p_rdl,Y
            adc         n
            sta         p_rdl,Y
            lda         p_rdh,Y
            adc         n + 1
            and         #>(PIPE_SIZE - 1)
            sta         p_rdh,Y
            sec
            lda         p_cntl,Y
            sbc         n
            sta         p_cntl,Y
            lda         p_cnth,Y
            sbc         n + 1
            sta         p_cnth,Y
            bra         p_done

; A write: as much as there's room for, as far as the buffer's end; no reader left: E_PIPE; no room: E_AGAIN
p_write:
            lda         pend
            beq         @badf
            ldy         pp
            lda         p_readers,Y
            beq         @broken
            sec                                             ; m: the room ...
            lda         #<PIPE_SIZE
            sbc         p_cntl,Y
            sta         m
            lda         #>PIPE_SIZE
            sbc         p_cnth,Y
            sta         m + 1
            ora         m
            bne         @room
            jmp         p_again

@badf:
            jmp         p_badf

@broken:
            jmp         p_broken

@room:
            sec                                             ; n: as far as the buffer's end
            lda         #<PIPE_SIZE
            sbc         p_wrl,Y
            sta         n
            lda         #>PIPE_SIZE
            sbc         p_wrh,Y
            sta         n + 1
            jsr         least
            lda         p_wrl,Y                             ; To the write place
            ldx         p_wrh,Y
            jsr         p_at
            MOVR        r1, TASK_INBOX + RQ_BUF
            MOVR        r2, n
            jsr         CLIENT_READ
            ldy         pp
            clc                                             ; The write place on (round), the count up
            lda         p_wrl,Y
            adc         n
            sta         p_wrl,Y
            lda         p_wrh,Y
            adc         n + 1
            and         #>(PIPE_SIZE - 1)
            sta         p_wrh,Y
            clc
            lda         p_cntl,Y
            adc         n
            sta         p_cntl,Y
            lda         p_cnth,Y
            adc         n + 1
            sta         p_cnth,Y
p_done:
            MOVR        TASK_INBOX + RQ_DONE, n
            inc         TASK_EVENT                          ; (The other end's waiters look again)
            clc
            rts

; n = the least of n, m and the request's count.  Keeps .Y
least:
            lda         m
            cmp         n
            lda         m + 1
            sbc         n + 1
            bcs         :+
            MOVR        n, m
:
            lda         TASK_INBOX + RQ_COUNT
            cmp         n
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         n + 1
            bcs         :+
            MOVR        n, TASK_INBOX + RQ_COUNT
:
            rts

; r0 = pipe pp's buffer + .A/.X
p_at:
            clc
            adc         #<p_buf
            sta         r0
            txa
            adc         #>p_buf
            sta         r0 + 1
            lda         pp                                  ; + pp * 512
            asl
            clc
            adc         r0 + 1
            sta         r0 + 1
            rts

.assert     PIPE_SIZE = 512, error, "p_at: a pipe's buffer is 512 bytes"

.rodata
zeros:      .res        64, 0
STATES      = 9
state_words: .word      s_free, s_ready, s_wait, s_call, s_idle, s_new, s_sleep, s_blocked, s_event
s_free:     .byte       "free", 0
s_ready:    .byte       "ready", 0
s_wait:     .byte       "wait", 0
s_call:     .byte       "call", 0
s_idle:     .byte       "idle", 0
s_new:      .byte       "new", 0
s_sleep:    .byte       "sleep", 0
s_blocked:  .byte       "blocked", 0
s_event:    .byte       "event", 0

; ****************************************************************************
; The devices
SRV_TREES:
            .byte       '/'
            .word       tree_root
            .byte       'n'
            .word       tree_null
            .byte       't'
            .word       tree_time
            .byte       'm'
            .word       tree_mods
            .byte       'p'
            .word       tree_procs
            .byte       '|'
            .word       tree_pipe
            .byte       'e'
            .word       tree_env
            .byte       0

tree_root:
            SRV_ENTRY   s_slash,   $FF, SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_bin,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_dev,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_env,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_lib,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_mnt,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_pc,      0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_proc,    0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_ram,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_rom,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_sd,      0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_sram,    0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_tmp,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_gpio,    2,   SK_DIR,  0,         SM_READ,            0     ; (In dev: its mount points)
            SRV_ENTRY   s_i2c,     2,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_mod,     2,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_sd,      2,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_spi,     2,   SK_DIR,  0,         SM_READ,            0
            .word       0
tree_null:
            SRV_ENTRY   s_slash,   $FF, SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_null,    0,   SK_DATA, h_null,    SM_READ | SM_WRITE, 0
            SRV_ENTRY   s_zero,    0,   SK_DATA, h_zero,    SM_READ | SM_WRITE, 0
            .word       0
tree_time:
            SRV_ENTRY   s_slash,   $FF, SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_ticks,   0,   SK_TEXT, gen_ticks, SM_READ,            0
            .word       0
tree_mods:
            SRV_ENTRY   s_slash,   $FF, SK_DYN,  h_mods,    SM_READ,            1
            SRV_ENTRY   s_slash,   SE_TEMPLATE, SK_DATA, h_image, SM_READ,      0     ; (Each module's file)
            SRV_ENTRY   s_bin,     0,   SK_DYN,  h_bins,    SM_READ,            3     ; bin
            SRV_ENTRY   s_slash,   SE_TEMPLATE, SK_DATA, h_image, SM_READ,      0     ; (Each program's file)
            .word       0
tree_procs:
            SRV_ENTRY   s_slash,   $FF, SK_DYN,  h_procs,   SM_READ,            1
            SRV_ENTRY   s_slash,   SE_TEMPLATE, SK_DIR, 0,  SM_READ,            0     ; (Each task's directory)
            SRV_ENTRY   s_status,  1,   SK_TEXT, gen_status, SM_READ,           0
            SRV_ENTRY   s_ctl,     1,   SK_CTL,  proc_cmds, SM_WRITE,           0
            .word       0
tree_pipe:
            SRV_ENTRY   s_slash,   $FF, SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_pipe,    0,   SK_DATA, h_pipe,    SM_READ | SM_WRITE, 0
            .word       0
tree_env:
            SRV_ENTRY   s_slash,   $FF, SK_RAW,  h_env,     SM_READ | SM_WRITE, 0
            .word       0
e_reqs:     .byte       R_OPEN, R_CREATE, R_READ, R_WRITE, R_CLUNK, R_STAT, R_REMOVE, R_FLUSH, R_DUP
E_NREQ      = * - e_reqs
e_reqvec:   .word       e_open, e_create, e_read, e_write, e_clunk, e_stat, e_remove, e_ok, e_dup
proc_cmds:
            .word       s_kill, c_kill
            .word       s_interrupt, c_intr
            .word       s_note, c_note
            .word       0
s_slash:    .byte       "/", 0
s_bin:      .byte       "bin", 0
s_dev:      .byte       "dev", 0
s_env:      .byte       "env", 0
s_lib:      .byte       "lib", 0
s_mnt:      .byte       "mnt", 0
s_pc:       .byte       "pc", 0
s_proc:     .byte       "proc", 0
s_ram:      .byte       "ram", 0
s_rom:      .byte       "rom", 0
s_sd:       .byte       "sd", 0
s_sram:     .byte       "sram", 0
s_tmp:      .byte       "tmp", 0
s_gpio:     .byte       "gpio", 0
s_i2c:      .byte       "i2c", 0
s_mod:      .byte       "mod", 0
s_spi:      .byte       "spi", 0
s_null:     .byte       "null", 0
s_zero:     .byte       "zero", 0
s_ticks:    .byte       "ticks", 0
s_status:   .byte       "status", 0
s_ctl:      .byte       "ctl", 0
s_pipe:     .byte       "pipe", 0
s_kill:     .byte       "kill", 0
s_interrupt: .byte      "interrupt", 0
s_note:     .byte       "note", 0

.include "srvlib.s"
