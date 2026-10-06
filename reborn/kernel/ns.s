; ****************************************************************************
; ns.s - namespaces, the kernel task's side (BIOS ROM page 3: KCALLs; docs/reimplementation-from-scratch.md, §13).
;
; A namespace is the mount entries with its number (K_MT_*); a task names one (K_TASK_NS), and tasks share one: a
; child gets its parent's (SPAWN), or an empty one (SPAWN_NEWNS), and a task that changes a shared one gets a copy
; first.  An entry is one member of the union at a mount point: a server's device letter and spec, and a path in
; that server; a union's members are in order (K_MT_SEQ, lowest first).  Mount points and paths are strings in a
; pool (K_SP), shared, and counted by the entries (and union channels) using them.
;
; A bind is resolved when it's made, as in Plan 9: "bind /rom/bin /bin" binds the server and path /rom/bin is then
; (the candidate its caller found: K_CAND), and binding a mount point itself binds all of its members.  So a name
; is resolved in one step: its longest mount point (whole elements), then that union's members in order, each a
; candidate its caller tries (file.s): the member's path, and the rest of the name.  A # name is its device's own
; (no namespace): one candidate.
;
; The caller (file.s, in the calling task) cleans the name into its TA_PATH; K_NS_FIND_K takes it (K_RES) and
; finds its mount point; K_NS_NEXT_K gives the candidates back, one a call, in the caller's TA_PATH and request
; block.  All are KCALLs (.Y = the caller), so the tables need no locks.

.include "kdefs.inc"

.assert     RQ_SPEC = RQ_DEV + 1, error, "K_CAND_DS is RQ_DEV and RQ_SPEC, as they are in a request block"
.assert     K0_END <= K0_POST, error, "The kernel task's zero page runs into POST's"
.assert     <K_RES = 0 .and <K_CAND = 0 .and <K_SP = 0, error, "t64 and sp_addr: the tables start on a page"

.segment "KCODE_P3"

; ****************************************************************************
; Setting up, inheriting, ending

; At boot (IRQs off): no namespaces, entries, strings or union channels
K_NS_INIT:
            ldx         #NS_MAX - 1
:
            stz         K_NS_REFS,X
            dex
            bpl         :-
            lda         #$FF
            ldx         #TASKS - 1
:
            sta         K_TASK_NS,X
            dex
            bpl         :-
            ldx         #MT_MAX - 1
:
            sta         K_MT_NS,X
            dex
            cpx         #$FF
            bne         :-
            ldx         #CH_MAX - 1
:
            sta         K_CH_UFROM,X
            dex
            bpl         :-
            ldx         #SP_MAX - 1
:
            stz         K_SP_REFS,X
            dex
            bpl         :-
            rts

; Task .X gets an empty namespace of its own (init, at boot; SPAWN_NEWNS), or none if there's no room.  Keeps .X
K_NS_FRESH:
            jsr         ns_new
            bcc         :+
            lda         #$FF
:
            sta         K_TASK_NS,X
            rts

; A child's namespace (FARCALL from K_SPAWN_K: .X = the child, K0_TMP3 = its parent, K0_SPAWNF = SPAWN's flags):
; its parent's, shared, or (SPAWN_NEWNS) an empty one.  Keeps .X
K_NS_INHERIT:
            lda         K0_SPAWNF
            and         #SPAWN_NEWNS
            bne         K_NS_FRESH
            ldy         K0_TMP3
            lda         K_TASK_NS,Y
            sta         K_TASK_NS,X
            cmp         #NS_MAX
            bcs         :+
            phx
            tax
            inc         K_NS_REFS,X
            plx
:
            rts

; Task .Y has ended (FARCALL from K_EXIT_K): its namespace, one user fewer.  Keeps .Y
K_NS_EXIT:
            lda         K_TASK_NS,Y
            pha
            lda         #$FF
            sta         K_TASK_NS,Y
            pla
            phy
            jsr         ns_rel
            ply
            rts

; ****************************************************************************
; Resolving a name

; KCALL: the caller's name (its TA_PATH: clean, absolute or a # name) into its K_RES, and its mount point found.
; OUT: C = 0, .A = the candidates there are (1 for a # name), .X <> 0 if the name is the mount point itself; or
; C = 1, .A = E_NOENT (no mount point: no namespace, or nothing mounted at /)
K_NS_FIND_K:
            sty         K0_NC
            tya
            ldx         #>K_RES
            jsr         t64                                 ; Its K_RES: the name, from its TA_PATH
            lda         K0_SC
            sta         K0_SB
            sta         K_PTR
            lda         K0_SC + 1
            sta         K0_SB + 1
            sta         K_PTR + 1
            lda         #<TA_PATH
            sta         K_PTR2
            lda         #>TA_PATH
            sta         K_PTR2 + 1
            lda         #PATH_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_NC
            sec
            FARCALL     K_KCOPY
            ldy         K0_NC
            lda         #0
            sta         K_RES_SEQ,Y
            lda         (K0_SB)                             ; A # name: its device's
            cmp         #'#'
            bne         @ns
            lda         #RS_DEV
            sta         K_RES_STATE,Y
            lda         #1
            ldx         #0
            clc
            rts

@ns:
            lda         K_TASK_NS,Y
            sta         K_RES_NS,Y
            sta         K0_NN
            cmp         #NS_MAX
            bcs         @noent
            lda         #$FF                                ; The longest mount point it starts with
            sta         K0_NF
            ldx         #MT_MAX - 1
@entry:
            lda         K_MT_NS,X
            cmp         K0_NN
            bne         @next
            lda         K_MT_FROM,X
            cmp         K0_NF
            beq         @next                               ; (The longest so far already)
            jsr         sp_addr
            jsr         match                               ; .A: its length
            bcs         @next
            ldy         K0_NF
            cpy         #$FF
            beq         @take
            cmp         K0_NL
            bcc         @next
@take:
            sta         K0_NL
            lda         K_MT_FROM,X
            sta         K0_NF
@next:
            dex
            cpx         #$FF
            bne         @entry
            lda         K0_NF
            cmp         #$FF
            beq         @noent
            ldy         K0_NC
            sta         K_RES_FROM,Y
            lda         K0_NL
            sta         K_RES_LEN,Y
            lda         #RS_NS
            sta         K_RES_STATE,Y
            lda         K0_NN                               ; Its members
            sta         K0_PN
            lda         K0_NF
            sta         K0_PF
            jsr         members
            pha
            ldy         K0_NL                               ; The name the mount point itself?  (Nothing after it;
            lda         (K0_SB),Y                           ;   for /, nothing after the /)
            beq         @exact
            cpy         #0
            bne         @within
            ldy         #1
            lda         (K0_SB),Y
            beq         @exact
@within:
            ldx         #0
            bra         @found

@exact:
            ldx         #1
@found:
            pla
            clc
            rts

@noent:
            ldy         K0_NC
            lda         #RS_NONE
            sta         K_RES_STATE,Y
            FAIL        E_NOENT

; KCALL: the caller's next candidate (after K_NS_FIND_K): its path in its server (the member's path and the rest of
; the name) into the caller's TA_PATH, its device and spec into its request block (RQ_DEV, RQ_SPEC, RQ_NAMELEN), and
; kept (K_CAND, K_CAND_DS: BIND's new).  IN: .A = 0: the next member; 1: the member for a CREATE (the first with
; MCREATE, or else the first), and no more after it.  OUT: C = 0, .A = its server's task, .X = its flags; or C = 1,
; .A = E_NOENT (no more), E_NODEV (its server's gone), E_NAMETOOLONG
K_NS_NEXT_K:
            sty         K0_NC
            sta         K0_NV
            tya
            ldx         #>K_RES
            jsr         t64
            lda         K0_SC                               ; K0_SB: the name
            sta         K0_SB
            lda         K0_SC + 1
            sta         K0_SB + 1
            lda         K_RES_STATE,Y
            cmp         #RS_DEV
            beq         @dev
            cmp         #RS_NS
            bne         @noent
            lda         K_RES_NS,Y
            sta         K0_PN
            lda         K_RES_FROM,Y
            sta         K0_PF
            lda         K0_NV
            bne         @create
            lda         K_RES_SEQ,Y                         ; The next member
            sta         K0_PA
            jsr         pick
            bcs         @end
            ldy         K0_NC
            sta         K_RES_SEQ,Y
            bra         @member

@create:                                                    ; A CREATE's: the first with MCREATE, or the first
            lda         #RS_DONE
            sta         K_RES_STATE,Y
            stz         K0_PA
:
            jsr         pick
            bcs         @first
            sta         K0_PA
            lda         K_MT_FLAGS,X
            and         #MCREATE
            beq         :-
            bra         @member

@first:
            stz         K0_PA
            jsr         pick
            bcs         @noent
@member:                                                    ; Entry .X, and the name past its mount point
            ldy         K0_NC
            lda         K_RES_LEN,Y
            clc
            adc         K0_SB
            sta         K0_SB
            bcc         :+
            inc         K0_SB + 1
:
            jmp         cand_entry

@dev:                                                       ; A # name: its device, and the rest after "#x"
            lda         #RS_DONE
            sta         K_RES_STATE,Y
            jmp         cand_dev

@end:
            ldy         K0_NC
            lda         #RS_DONE
            sta         K_RES_STATE,Y
@noent:
            FAIL        E_NOENT

; KCALL: the caller's current directory (CHDIR): its last name resolved (K_RES) into its TA_CWD
K_NS_CWD_K:
            tya
            ldx         #>K_RES
            jsr         t64
            lda         K0_SC
            sta         K_PTR
            lda         K0_SC + 1
            sta         K_PTR + 1
            lda         #<TA_CWD
            sta         K_PTR2
            lda         #>TA_CWD
            sta         K_PTR2 + 1
            lda         #PATH_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            tya
            clc
            FARCALL     K_KCOPY
            clc
            rts

; ****************************************************************************
; Union directories: a channel opened on a mount point with more members than one reads them all, in turn

; KCALL: channel .A, just opened by the caller on its last name's first member: a union directory now (its
; namespace and mount point kept, and counted)
K_NS_UNION_K:
            tax
            lda         K_RES_STATE,Y
            cmp         #RS_NS
            bne         @done
            lda         K_RES_FROM,Y
            sta         K_CH_UFROM,X
            jsr         sp_ref
            lda         K_RES_SEQ,Y
            sta         K_CH_USEQ,X
            lda         K_RES_NS,Y
            sta         K_CH_UNS,X
            tax
            inc         K_NS_REFS,X
@done:
            clc
            rts

; KCALL: channel .A, a union directory, at the end of a member: the next member, as a candidate (as K_NS_NEXT_K's)
; for the caller to open in its place.  OUT: as K_NS_NEXT_K's (E_NOENT: no more, or not a union)
K_NS_UNEXT_K:
            sty         K0_NC
            tax
            lda         K_CH_UFROM,X
            cmp         #$FF
            beq         @noent
            sta         K0_PF
            lda         K_CH_UNS,X
            sta         K0_PN
            lda         K_CH_USEQ,X
            sta         K0_PA
            phx
            jsr         pick
            ply                                             ; (.Y: the channel)
            bcs         @noent
            sta         K_CH_USEQ,Y
            lda         #<s_empty                           ; (No rest: the member's own directory)
            sta         K0_SB
            lda         #>s_empty
            sta         K0_SB + 1
            jmp         cand_entry

@noent:
            FAIL        E_NOENT

; KCALL: channel .A has closed: if it was a union directory, its namespace and mount point, one user fewer
K_NS_UREL_K:
            tax
            lda         K_CH_UFROM,X
            cmp         #$FF
            beq         @done
            jsr         sp_rel
            lda         #$FF
            sta         K_CH_UFROM,X
            lda         K_CH_UNS,X
            jsr         ns_rel
@done:
            clc
            rts

; ****************************************************************************
; Changing a namespace

; KCALL: BIND.  The caller's TA_PATH = old (clean and absolute).  IN: .A = the flags (MREPL, MBEFORE, MAFTER; and
; MCREATE); .X = what's bound: 0 its last candidate (K_CAND), 1 the union at its last name's mount point.  OUT: C = 0;
; or C = 1, .A = E_INVAL (old isn't absolute; or it's the union bound), E_NSFULL (no room in the tables)
K_NS_BIND_K:
            sty         K0_NC
            sta         K0_NV
            stx         K0_NW
            lda         K_RES_NS,Y                          ; (The union, for .X = 1)
            sta         K0_NS1
            lda         K_RES_FROM,Y
            sta         K0_NS2
            jsr         ns_old                              ; Old, into K_NS_PATH
            bcs         ns_done
ns_bind:                                                    ; (MOUNT's way in)
            jsr         ns_own                              ; Its namespace: its own
            bcs         ns_done
            sta         K0_NN
            lda         #<K_NS_PATH                         ; Old's string (held till the end)
            sta         K0_SB
            lda         #>K_NS_PATH
            sta         K0_SB + 1
            jsr         sp_get
            bcs         ns_done
            sta         K0_NF
            lda         K0_NW                               ; (A union bound on itself: no)
            beq         :+
            lda         K0_NF
            cmp         K0_NS2
            bne         :+
            lda         #E_INVAL
            bra         @failed
:
            lda         K0_NV                               ; In place of what's there: that goes
            and         #MORDER
            bne         :+
            jsr         drop_all
:
            lda         K0_NN                               ; The lowest and highest seq there now
            sta         K0_PN
            lda         K0_NF
            sta         K0_PF
            jsr         seq_range
            lda         #1                                  ; How many go in
            ldx         K0_NW
            beq         :+
            lda         K0_NS1
            sta         K0_PN
            lda         K0_NS2
            sta         K0_PF
            jsr         members
:
            sta         K0_NT
            jsr         seq_first                           ; K0_NQS: the first's seq
            bcs         @failed
            lda         K0_NW
            bne         @union
            jsr         add_cand
            bra         @end

@union:                                                     ; The source union's members, in order
            stz         K0_PA
:
            jsr         pick
            bcs         @copied
            sta         K0_PA
            jsr         add_copy
            bcc         :-
            bra         @end                                ; (No room: E_NSFULL)

@copied:
            clc
@end:
            php                                             ; (Old's string: as many users as entries now)
            pha
            lda         K0_NF
            jsr         sp_rel
            pla
            plp
            rts

@failed:
            pha
            lda         K0_NF
            jsr         sp_rel
            pla
            sec
ns_done:
            rts

; KCALL: MOUNT.  The caller's TA_PATH = old; its TA_SCRATCH = the spec (8 bytes, zero-padded).  IN: .A = the flags
; (as BIND's); .X = the device letter.  OUT: as BIND's, and E_NODEV
K_NS_MOUNT_K:
            sty         K0_NC
            sta         K0_NV
            stz         K0_NW
            txa
            pha
            FARCALL     K_DEV_FIND_K                        ; (Its server: there?)
            bcc         :+
            plx
            rts

:
            lda         K0_NC                               ; The candidate: the device's root, with the spec
            asl
            asl
            asl
            asl
            tax
            pla
            sta         K_CAND_DS,X
            txa
            clc
            adc         #<(K_CAND_DS + 1)
            sta         K_PTR
            lda         #>(K_CAND_DS + 1)
            adc         #0
            sta         K_PTR + 1
            lda         #<TA_SCRATCH
            sta         K_PTR2
            lda         #>TA_SCRATCH
            sta         K_PTR2 + 1
            lda         #8
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_NC
            sec
            FARCALL     K_KCOPY
            lda         K0_NC
            ldx         #>K_CAND
            jsr         t64
            lda         #0
            sta         (K0_SC)
            jsr         ns_old
            bcs         ns_done
            jmp         ns_bind

; KCALL: UNMOUNT.  The caller's TA_PATH = old.  IN: .X = 0: all of the union there; 1: the members that are its
; last candidate (K_CAND).  OUT: C = 0; or C = 1, .A = E_NOENT (none of them there), E_INVAL, E_NSFULL
K_NS_UNMOUNT_K:
            sty         K0_NC
            stx         K0_NW
            jsr         ns_old
            bcs         @done
            jsr         ns_own
            bcs         @done
            sta         K0_NN
            lda         #<K_NS_PATH                         ; Old's string, if there is one
            sta         K0_SB
            lda         #>K_NS_PATH
            sta         K0_SB + 1
            jsr         sp_find
            bcs         @noent
            sta         K0_NF
            lda         K0_NW
            bne         :+
            jsr         drop_all
            bra         @count
:
            lda         K0_NC                               ; The candidate's path's string, if there is one
            ldx         #>K_CAND
            jsr         t64
            lda         K0_SC
            sta         K0_SB
            lda         K0_SC + 1
            sta         K0_SB + 1
            jsr         sp_find
            bcs         @noent
            sta         K0_NT
            lda         K0_NC
            asl
            asl
            asl
            asl
            sta         K0_NL                               ; (Its K_CAND_DS)
            stz         K0_NV                               ; (Dropped)
            ldx         #MT_MAX - 1
@entry:
            lda         K_MT_NS,X
            cmp         K0_NN
            bne         @next
            lda         K_MT_FROM,X
            cmp         K0_NF
            bne         @next
            lda         K_MT_PATH,X
            cmp         K0_NT
            bne         @next
            jsr         same_ds
            bne         @next
            jsr         mt_free
            inc         K0_NV
@next:
            dex
            cpx         #$FF
            bne         @entry
            lda         K0_NV
@count:
            beq         @noent
            clc
@done:
            rts

@noent:
            FAIL        E_NOENT

; Z = 1 if entry .X's device and spec are the candidate's (K_CAND_DS + K0_NL).  Keeps .X
same_ds:
            ldy         K0_NL
            lda         K_MT_DEV,X
            cmp         K_CAND_DS,Y
            bne         @done
            .repeat     8, I
            lda         K_MT_SPEC + I * MT_MAX,X
            cmp         K_CAND_DS + 1 + I,Y
            bne         @done
            .endrepeat
@done:
            rts

; ****************************************************************************
; The pieces

; Old: the caller's TA_PATH into K_NS_PATH; it must be absolute.  OUT: C = 0; or C = 1, .A = E_INVAL
ns_old:
            lda         #<K_NS_PATH
            sta         K_PTR
            lda         #>K_NS_PATH
            sta         K_PTR + 1
            lda         #<TA_PATH
            sta         K_PTR2
            lda         #>TA_PATH
            sta         K_PTR2 + 1
            lda         #PATH_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_NC
            sec
            FARCALL     K_KCOPY
            lda         K_NS_PATH
            cmp         #'/'
            bne         @inval
            clc
            rts

@inval:
            FAIL        E_INVAL

; The candidate: entry .X's device, spec and path, then the rest of the name at K0_SB (a /, or nothing), into the
; caller's K_CAND and K_CAND_DS, and to the caller (cand_out).  .X's flags go back with it
cand_entry:
            lda         K_MT_FLAGS,X
            sta         K0_NT
            lda         K0_NC
            asl
            asl
            asl
            asl
            tay
            lda         K_MT_DEV,X
            sta         K_CAND_DS,Y
            .repeat     8, I
            lda         K_MT_SPEC + I * MT_MAX,X
            sta         K_CAND_DS + 1 + I,Y
            .endrepeat
            lda         K_MT_PATH,X
            jsr         sp_addr                             ; K0_SA: its path
            bra         cand_path

; A # name's candidate: its device (the name's second character), its spec (what's between that and the /, as
; Plan 9's #I1: 8 characters at most, f_clean saw to that), and the rest
cand_dev:
            stz         K0_NT
            lda         K0_NC
            asl
            asl
            asl
            asl
            tax
            ldy         #1
            lda         (K0_SB),Y
            sta         K_CAND_DS,X
            .repeat     8, I
            stz         K_CAND_DS + 1 + I,X
            .endrepeat
            iny
@spec:
            lda         (K0_SB),Y
            beq         @rest
            cmp         #'/'
            beq         @rest
            sta         K_CAND_DS + 1,X
            inx
            iny
            cpy         #2 + 8
            bcc         @spec
@rest:
            tya                                             ; The rest
            clc
            adc         K0_SB
            sta         K0_SB
            bcc         :+
            inc         K0_SB + 1
:
            lda         #<s_empty
            sta         K0_SA
            lda         #>s_empty
            sta         K0_SA + 1
; The path K0_SA, then the rest K0_SB (past its leading /s if the path is empty), into the caller's K_CAND
cand_path:
            lda         K0_NC
            ldx         #>K_CAND
            jsr         t64                                 ; K0_SC: its K_CAND
            ldy         #0
@path:
            lda         (K0_SA),Y
            beq         @rest
            sta         (K0_SC),Y
            iny
            cpy         #PATH_MAX
            bcc         @path
@long:
            FAIL        E_NAMETOOLONG

@rest:
            sty         K0_NL                               ; (The path's length)
            tya
            bne         @join
@slash:
            lda         (K0_SB)                             ; (No path: the rest's leading /s go)
            cmp         #'/'
            bne         @join
            inc         K0_SB
            bne         @slash
            inc         K0_SB + 1
            bra         @slash

@join:
            clc                                             ; K0_SC: where the rest goes
            lda         K0_SC
            adc         K0_NL
            sta         K0_SC
            bcc         :+
            inc         K0_SC + 1
:
            ldy         #0
@copy:
            lda         (K0_SB),Y
            sta         (K0_SC),Y
            beq         @end
            iny
            tya
            clc
            adc         K0_NL
            cmp         #PATH_MAX + 1
            bcc         @copy
            bra         @long

@end:
            tya                                             ; K0_NL: the whole length
            clc
            adc         K0_NL
            sta         K0_NL
; The candidate (K_CAND, K_CAND_DS; K0_NL its path's length, K0_NT its flags) to the caller: its TA_PATH, RQ_NAMELEN,
; RQ_DEV and RQ_SPEC.  OUT: C = 0, .A = its server, .X = its flags; or C = 1, .A = E_NODEV
cand_out:
            lda         K0_NC
            ldx         #>K_CAND
            jsr         t64
            lda         K0_SC
            sta         K_PTR
            lda         K0_SC + 1
            sta         K_PTR + 1
            lda         #<TA_PATH
            sta         K_PTR2
            lda         #>TA_PATH
            sta         K_PTR2 + 1
            lda         #PATH_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_NC
            clc
            FARCALL     K_KCOPY
            lda         K0_NC                               ; Its device and spec
            asl
            asl
            asl
            asl
            clc
            adc         #<K_CAND_DS
            sta         K_PTR
            lda         #>K_CAND_DS
            adc         #0
            sta         K_PTR + 1
            lda         #<(TA_REQ + RQ_DEV)
            sta         K_PTR2
            lda         #>(TA_REQ + RQ_DEV)
            sta         K_PTR2 + 1
            lda         #9
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_NC
            clc
            FARCALL     K_KCOPY
            ldx         K0_NC                               ; Its name's length
            ldy         T_REGISTER
            php
            sei
            lda         K0_NL
            QL_PUT      TA_REQ + RQ_NAMELEN
            plp
            lda         K0_NC                               ; Its server
            asl
            asl
            asl
            asl
            tax
            lda         K_CAND_DS,X
            FARCALL     K_DEV_FIND_K
            ldx         K0_NT
            rts

; Does the name at K0_SB start with the mount point at K0_SA (whole elements)?  OUT: C = 0, .A = the length that
; matched (0 for /); or C = 1.  Keeps .X
match:
            ldy         #1
            lda         (K0_SA),Y
            bne         @path
            lda         (K0_SB)                             ; / is the start of every absolute name
            cmp         #'/'
            bne         @no
            lda         #0
            clc
            rts

@path:
            ldy         #0
:
            lda         (K0_SA),Y
            beq         @end
            cmp         (K0_SB),Y
            bne         @no
            iny
            bra         :-

@end:
            lda         (K0_SB),Y                           ; (The name's element ends there too)
            beq         @yes
            cmp         #'/'
            bne         @no
@yes:
            tya
            clc
            rts

@no:
            sec
            rts

; The member of the union K0_PN, K0_PF with the lowest seq after K0_PA (0: the first).  OUT: C = 0, .X = its entry,
; .A = its seq; or C = 1: none.  Keeps .Y
pick:
            lda         #$FF
            sta         K0_PB
            sta         K0_PE
            ldx         #MT_MAX - 1
@entry:
            lda         K_MT_NS,X
            cmp         K0_PN
            bne         @next
            lda         K_MT_FROM,X
            cmp         K0_PF
            bne         @next
            lda         K_MT_SEQ,X
            cmp         K0_PA
            bcc         @next
            beq         @next
            cmp         K0_PB
            bcs         @next
            sta         K0_PB
            stx         K0_PE
@next:
            dex
            cpx         #$FF
            bne         @entry
            ldx         K0_PE
            cpx         #$FF
            beq         @none
            lda         K0_PB
            clc
            rts

@none:
            sec
            rts

; .A = the members of the union K0_PN, K0_PF.  Keeps .Y
members:
            stz         K0_NT
            ldx         #MT_MAX - 1
@entry:
            lda         K_MT_NS,X
            cmp         K0_PN
            bne         @next
            lda         K_MT_FROM,X
            cmp         K0_PF
            bne         @next
            inc         K0_NT
@next:
            dex
            cpx         #$FF
            bne         @entry
            lda         K0_NT
            rts

; K0_C1 and K0_C2 = the lowest and highest seq of the union K0_PN, K0_PF (none: 255 and 0)
seq_range:
            lda         #$FF
            sta         K0_C1
            stz         K0_C2
            ldx         #MT_MAX - 1
@entry:
            lda         K_MT_NS,X
            cmp         K0_PN
            bne         @next
            lda         K_MT_FROM,X
            cmp         K0_PF
            bne         @next
            lda         K_MT_SEQ,X
            cmp         K0_C1
            bcs         :+
            sta         K0_C1
:
            cmp         K0_C2
            bcc         @next
            sta         K0_C2
@next:
            dex
            cpx         #$FF
            bne         @entry
            rts

; K0_NQS = the first seq for K0_NT new members (by the flags in K0_NV: before what's there, after it, or in its place:
; 128 when there's nothing).  OUT: C = 0; or C = 1, .A = E_NSFULL (the seqs run out: 1-254)
seq_first:
            lda         K0_C2
            beq         @alone                              ; (Nothing there)
            lda         K0_NV
            and         #MORDER
            cmp         #MBEFORE
            beq         @before
            sec                                             ; After: the highest + 1, and room for them all
            lda         K0_C2
            adc         #0
            sta         K0_NQS
            clc
            adc         K0_NT
            bcs         @full
            cmp         #255
            bcs         @full
            clc
            rts

@before:                                                    ; Before: the lowest - their count
            sec
            lda         K0_C1
            sbc         K0_NT
            bcc         @full
            beq         @full
            sta         K0_NQS
            clc
            rts

@alone:
            lda         #128
            sta         K0_NQS
            clc
            rts

@full:
            FAIL        E_NSFULL

; Every member of the union K0_NN, K0_NF dropped.  OUT: .A = how many (Z = 1: none)
drop_all:
            stz         K0_NT
            ldx         #MT_MAX - 1
@entry:
            lda         K_MT_NS,X
            cmp         K0_NN
            bne         @next
            lda         K_MT_FROM,X
            cmp         K0_NF
            bne         @next
            jsr         mt_free
            inc         K0_NT
@next:
            dex
            cpx         #$FF
            bne         @entry
            lda         K0_NT
            rts

; A new member of the union K0_NN, K0_NF, seq K0_NQS (then the next): the caller's candidate (add_cand), or a copy
; of entry .X (add_copy), with the flags' MCREATE.  OUT: C = 0; or C = 1, .A = E_NSFULL
add_cand:
            jsr         mt_new
            bcs         @done
            jsr         add_common
            lda         K0_NC                               ; Its device and spec: the candidate's
            asl
            asl
            asl
            asl
            tax
            lda         K_CAND_DS,X
            sta         K_MT_DEV,Y
            .repeat     8, I
            lda         K_CAND_DS + 1 + I,X
            sta         K_MT_SPEC + I * MT_MAX,Y
            .endrepeat
            lda         #0
            sta         K_MT_FLAGS,Y
            phy
            lda         K0_NC                               ; Its path: the candidate's
            ldx         #>K_CAND
            jsr         t64
            lda         K0_SC
            sta         K0_SB
            lda         K0_SC + 1
            sta         K0_SB + 1
            jsr         sp_get
            ply
            bcs         @undo
            sta         K_MT_PATH,Y
            lda         K0_NV
            and         #MCREATE
            sta         K_MT_FLAGS,Y
            clc
@done:
            rts

@undo:
            pha
            lda         #$FF
            sta         K_MT_NS,Y
            lda         K0_NF
            jsr         sp_rel
            pla
            sec
            rts

add_copy:
            jsr         mt_new
            bcs         @done
            jsr         add_common
            lda         K_MT_DEV,X
            sta         K_MT_DEV,Y
            .repeat     8, I
            lda         K_MT_SPEC + I * MT_MAX,X
            sta         K_MT_SPEC + I * MT_MAX,Y
            .endrepeat
            lda         K_MT_PATH,X
            sta         K_MT_PATH,Y
            jsr         sp_ref
            lda         K0_NV
            and         #MCREATE
            ora         K_MT_FLAGS,X
            sta         K_MT_FLAGS,Y
            clc
@done:
            rts

; Entry .Y: in the union K0_NN, K0_NF (the mount point's string counted), with seq K0_NQS (then the next).  Keeps .X
add_common:
            lda         K0_NN
            sta         K_MT_NS,Y
            lda         K0_NF
            sta         K_MT_FROM,Y
            jsr         sp_ref
            lda         K0_NQS
            sta         K_MT_SEQ,Y
            inc         K0_NQS
            rts

; The caller's namespace (K0_NC), made its own: a new empty one if it has none, a copy if it's shared.  OUT: C = 0,
; .A = it; or C = 1, .A = E_NSFULL
ns_own:
            ldy         K0_NC
            lda         K_TASK_NS,Y
            cmp         #NS_MAX
            bcs         @new
            tax
            lda         K_NS_REFS,X
            cmp         #2
            bcs         @copy
            txa
            clc
            rts

@new:
            jsr         ns_new
            bcs         @done
            ldy         K0_NC
            sta         K_TASK_NS,Y
            clc
@done:
            rts

@copy:
            txa
            jsr         ns_clone
            bcs         @done
            ldy         K0_NC
            pha
            ldx         K_TASK_NS,Y                         ; (The shared one: one user fewer)
            dec         K_NS_REFS,X
            pla
            sta         K_TASK_NS,Y
            clc
            rts

; A copy of namespace .A: a new one with the same entries.  OUT: C = 0, .A = it; or C = 1, .A = E_NSFULL
ns_clone:
            sta         K0_CO
            jsr         ns_new
            bcs         @done
            sta         K0_CN
            ldx         #MT_MAX - 1
@entry:
            lda         K_MT_NS,X
            cmp         K0_CO
            bne         @next
            jsr         mt_new
            bcs         @full
            lda         K0_CN
            sta         K_MT_NS,Y
            lda         K_MT_FROM,X
            sta         K_MT_FROM,Y
            jsr         sp_ref
            lda         K_MT_PATH,X
            sta         K_MT_PATH,Y
            jsr         sp_ref
            lda         K_MT_SEQ,X
            sta         K_MT_SEQ,Y
            lda         K_MT_DEV,X
            sta         K_MT_DEV,Y
            lda         K_MT_FLAGS,X
            sta         K_MT_FLAGS,Y
            .repeat     8, I
            lda         K_MT_SPEC + I * MT_MAX,X
            sta         K_MT_SPEC + I * MT_MAX,Y
            .endrepeat
@next:
            dex
            cpx         #$FF
            bne         @entry
            lda         K0_CN
            clc
@done:
            rts

@full:                                                      ; (No room: the copy undone)
            lda         K0_CN
            jsr         ns_rel
            FAIL        E_NSFULL

; A free namespace, its count 1.  OUT: C = 0, .A = it; or C = 1, .A = E_NSFULL.  Keeps .X
ns_new:
            ldy         #NS_MAX - 1
:
            lda         K_NS_REFS,Y
            beq         :+
            dey
            bpl         :-
            FAIL        E_NSFULL
:
            lda         #1
            sta         K_NS_REFS,Y
            tya
            clc
            rts

; Namespace .A ($FF: none): one user fewer; the last frees its entries.  Modifies .A, .X
ns_rel:
            cmp         #NS_MAX
            bcs         @done
            sta         K0_NR
            tax
            dec         K_NS_REFS,X
            bne         @done
            ldx         #MT_MAX - 1
@entry:
            lda         K_MT_NS,X
            cmp         K0_NR
            bne         :+
            jsr         mt_free
:
            dex
            cpx         #$FF
            bne         @entry
@done:
            rts

; .Y = a free entry.  OUT: C = 0; or C = 1, .A = E_NSFULL.  Keeps .X
mt_new:
            ldy         #MT_MAX - 1
:
            lda         K_MT_NS,Y
            cmp         #$FF
            beq         :+
            dey
            cpy         #$FF
            bne         :-
            FAIL        E_NSFULL
:
            clc
            rts

; Entry .X free, its strings one user fewer.  Keeps .X, .Y
mt_free:
            lda         K_MT_FROM,X
            jsr         sp_rel
            lda         K_MT_PATH,X
            jsr         sp_rel
            lda         #$FF
            sta         K_MT_NS,X
            rts

; ****************************************************************************
; The strings

; The string at K0_SB, in the pool: a slot that has it already, or a free one it's copied into; one user more.
; OUT: C = 0, .A = the slot; or C = 1, .A = E_NSFULL.  Keeps .X
sp_get:
            jsr         sp_find
            bcc         @found
            phx
            ldx         #SP_MAX - 1                         ; A free one
:
            lda         K_SP_REFS,X
            beq         :+
            dex
            bpl         :-
            plx
            FAIL        E_NSFULL
:
            txa
            jsr         sp_addr
            ldy         #0
:
            lda         (K0_SB),Y
            sta         (K0_SA),Y
            beq         :+
            iny
            cpy         #PATH_MAX
            bcc         :-
            lda         #0                                  ; (Never longer)
            sta         (K0_SA),Y
:
            txa
            plx
@found:
            jmp         sp_ref

; The slot with the string at K0_SB.  OUT: C = 0, .A = it; or C = 1: none.  Keeps .X
sp_find:
            phx
            ldx         #SP_MAX - 1
@slot:
            lda         K_SP_REFS,X
            beq         @next
            txa
            jsr         sp_addr
            jsr         str_same
            beq         @found
@next:
            dex
            bpl         @slot
            plx
            sec
            rts

@found:
            txa
            plx
            clc
            rts

; Z = 1 if the strings at K0_SA and K0_SB are the same.  Keeps .X
str_same:
            ldy         #0
:
            lda         (K0_SA),Y
            cmp         (K0_SB),Y
            bne         @done
            cmp         #0
            beq         @done
            iny
            bpl         :-
@done:
            rts

; ****************************************************************************
; NSINFO: a namespace's mount entry.  IN: .A = a task ($FF: this one); .X = which (0 on, in the table's order); r0 =
; a buffer (NI_SIZE bytes).  OUT: the entry in it (NI_*); or C = 1, .A = E_RANGE (past the last), E_SRCH
K_NSINFO:
            KCALL_FAR   K_NSINFO_K
            rts

; NSINFO's (a KCALL: .Y = the caller): the entry made in K_XBUF, then copied to the caller's buffer
K_NSINFO_K:
            sty         K0_TMP2                             ; (The caller)
            cmp         #$FF
            bne         :+
            tya
:
            cmp         #TASKS
            bcs         @srch
            stx         K0_TMP3                             ; (Which)
            tax
            lda         K_TASK_NS,X                         ; Its namespace's entries
            bmi         @range                              ; (None: none)
            sta         K0_TMP
            ldx         #0
@entry:
            lda         K_MT_NS,X
            cmp         K0_TMP
            bne         @next
            lda         K0_TMP3
            beq         @this
            dec         K0_TMP3
@next:
            inx
            cpx         #MT_MAX
            bcc         @entry
@range:
            FAIL        E_RANGE

@srch:
            FAIL        E_SRCH

@this:
            phx                                             ; Its mount point and path
            lda         K_MT_FROM,X
            ldx         #NI_FROM
            jsr         ni_str
            plx
            phx
            lda         K_MT_PATH,X
            ldx         #NI_PATH
            jsr         ni_str
            plx
            lda         K_MT_DEV,X                          ; Its device, spec, flags and place
            sta         K_XBUF + NI_DEV
            .repeat     8, I
            lda         K_MT_SPEC + I * MT_MAX,X
            sta         K_XBUF + NI_SPEC + I
            .endrepeat
            stz         K_XBUF + NI_SPEC + 8
            lda         K_MT_FLAGS,X
            sta         K_XBUF + NI_FLAGS
            lda         K_MT_SEQ,X
            sta         K_XBUF + NI_SEQ
            lda         #<K_XBUF                            ; To the caller's buffer
            sta         K_PTR
            lda         #>K_XBUF
            sta         K_PTR + 1
            ldx         K0_TMP2
            ldy         T_REGISTER
            php
            sei
            QL_GET      r0
            sta         K_PTR2
            QL_GET      r0 + 1
            sta         K_PTR2 + 1
            plp
            lda         #NI_SIZE
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_TMP2
            clc
            FARCALL     K_KCOPY
            clc
            rts

; String .A (the pool's) into K_XBUF at .X, zero-terminated
ni_str:
            jsr         sp_addr
            ldy         #0
:
            lda         (K0_SA),Y
            sta         K_XBUF,X
            beq         @done
            inx
            iny
            cpy         #PATH_MAX
            bne         :-
            stz         K_XBUF,X
@done:
            rts

; K0_SA = string .A.  Keeps .X, .Y
sp_addr:
            stz         K0_SA + 1
            .repeat     6
            asl
            rol         K0_SA + 1
            .endrepeat
            sta         K0_SA
            lda         K0_SA + 1
            clc
            adc         #>K_SP
            sta         K0_SA + 1
            rts

; String .A: one user more (sp_ref; C = 0), or fewer (sp_rel).  Keeps .A, .X, .Y
sp_ref:
            phx
            tax
            inc         K_SP_REFS,X
            txa
            plx
            clc
            rts

sp_rel:
            phx
            tax
            lda         K_SP_REFS,X
            beq         :+
            dec         K_SP_REFS,X
:
            txa
            plx
            rts

; K0_SC = task .A's 64 bytes in the table at page .X (K_RES, K_CAND).  Keeps .Y
t64:
            pha
            lsr
            lsr
            stx         K0_SC + 1
            clc
            adc         K0_SC + 1
            sta         K0_SC + 1
            pla
            and         #3
            lsr
            ror
            ror
            sta         K0_SC
            rts

.segment "KRODATA_P3"
s_empty:    .byte       0
