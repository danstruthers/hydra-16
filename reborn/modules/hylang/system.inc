; ****************************************************************************
; system.inc - hylang's system library (phase 7), in its fifth bank, as danlang's (SystemBuiltins.cs), on the
; Hydra's calls: files and directories (a directory's names sorted), glob (rc's patterns), programs and the shell
; (rc -c; a bare name is /bin's), the environment (rc's lists: a value of several words, each with a 0 after it, is
; a list of strings), the clock (seconds since 2000-01-01; ticks, TICK_HZ a second), and where it's running.  A
; failure is the system's error, about the first argument ("path: not found", :noent).

.bss
y_lroot:    .res        1                                   ; (y_names': the list's place on the root stack)
y_prev:     .res        2                                   ; (A sorted list's walk: the cons before, the cons at)
y_cur:      .res        2
y_new:      .res        2
y_off:      .res        1                                   ; (A directory's records: the one at)
y_cnt:      .res        1
y_pat:      .res        PATH_MAX + 1                        ; (glob's: a part of the pattern)
y_name32:   .res        32                                  ; (A name matched)
y_pi:       .res        1                                   ; (match's: places in the pattern and the name, and where
y_si:       .res        1                                   ;   a * was)
y_star:     .res        1
y_mark:     .res        1
y_part:     .res        1                                   ; (glob's: the part, its first byte in the pattern)
y_pos:      .res        1
y_any:      .res        1
y_task:     .res        1                                   ; (A task: spawn's, wait's)
y_pipe:     .res        4                                   ; (sh-out's pipes: input's read end, write end; output's)
y_map:      .res        4                                   ; (SPAWN's fd map)
y_date:     .res        8                                   ; (A date's parts: year (2), month, day, hour, minute,
                                                            ;   second, weekday)
y_days:     .res        2

.segment "CODE5"

; ---- Directories

; hv = the names in the directory y_path, sorted (a file: its own name).  OUT: C = 1 and .A = the system's error
; (ex not set) if there's none; cf, cr kept
y_names:
            lda         cf
            ldx         cf + 1
            jsr         y_rpush
            lda         cr
            ldx         cr + 1
            jsr         y_rpush
            lda         #0                                  ; (The list: on the root stack)
            tax
            jsr         y_rpush
            lda         rsp
            sta         y_lroot
            dec         y_lroot
            jsr         y_stat
            bcc         @lb415
            jmp         @fail
@lb415:
            jsr         y_isdir
            bne         @dir
            LDAX        y_buf + SR_NAME                     ; (A file: its name)
            jsr         y_insname
            bra         @done
@dir:
            lda         #O_READ
            jsr         y_open
            bcs         @fail
@chunk:
            LDR         r0, y_buf
            LDR         r1, 256
            lda         y_fd
            jsr         READ
            bcs         @readfail
            sta         y_cnt
            txa
            beq         :+
            lda         #0                                  ; (256)
            sta         y_cnt
:
            lda         y_cnt
            ora         #0
            bne         :+
            cpx         #0
            beq         @closed
:
            stz         y_off
@record:
            lda         y_off
            clc
            adc         #<y_buf
            pha
            lda         #>y_buf
            adc         #0
            tax
            pla
            jsr         y_insname
            lda         y_off
            clc
            adc         #SR_SIZE
            sta         y_off
            beq         @chunk                              ; (256: the chunk's end)
            ldx         y_cnt                               ; (256 read: on)
            beq         @record
            cmp         y_cnt
            bcc         @record
            bra         @chunk
@closed:
            lda         y_fd
            jsr         CLOSE
@done:
            jsr         root_pop
            sta         hv
            stx         hv + 1
            jsr         root_pop
            sta         cr
            stx         cr + 1
            jsr         root_pop
            sta         cf
            stx         cf + 1
            clc
            rts
@readfail:
            pha
            lda         y_fd
            jsr         CLOSE
            pla
@fail:
            pha
            lda         y_lroot                             ; (The root stack as it was, cf and cr back)
            sta         rsp
            jsr         root_pop
            sta         cr
            stx         cr + 1
            jsr         root_pop
            sta         cf
            stx         cf + 1
            pla
            sec
            rts

; The zero-terminated name at .A, .X (in RAM) made a string and put in order in y_names' list
y_insname:
            sta         hq
            stx         hq + 1
            ldy         #0
:
            lda         (hq),y
            beq         :+
            iny
            cpy         #32
            bcc         :-
:
            sty         hn
            stz         hn + 1
            jsr         string_make
            bcc         y_insstr
            jmp         y_nomem
; The string hv put in order in y_names' list
y_insstr:
            MOVW        et, hv                              ; (Its cons)
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         y_make2
            MOVW        y_new, hv
            stz         y_prev
            stz         y_prev + 1
            ldx         y_lroot
            lda         rs_lo,x
            ldy         rs_hi,x
@walk:
            sta         y_cur
            sty         y_cur + 1
            ora         y_cur + 1
            beq         @here
            lda         y_cur                               ; (Past those before it)
            ldx         y_cur + 1
            ldy         #0
            jsr         cell_get
            sta         q_a
            stx         q_a + 1
            lda         y_new
            ldx         y_new + 1
            ldy         #0
            jsr         cell_get
            sta         q_b
            stx         q_b + 1
            FARN        2, q_strcmp
            bcs         @here                               ; (It's after the new one: here)
            MOVW        y_prev, y_cur
            lda         y_cur
            ldx         y_cur + 1
            ldy         #1
            jsr         cell_get
            phx
            ply
            bra         @walk
@here:
            MOVW        hv, y_new                           ; (Its rest: the one it's before)
            lda         y_cur
            ldx         y_cur + 1
            ldy         #1
            jsr         cell_set
            lda         y_prev
            ora         y_prev + 1
            bne         :+
            ldx         y_lroot                             ; (The list's first)
            lda         y_new
            sta         rs_lo,x
            lda         y_new + 1
            sta         rs_hi,x
            rts
:
            MOVW        hv, y_prev
            lda         y_new
            ldx         y_new + 1
            ldy         #1
            jmp         cell_set

; hv = the stat record y_buf as a hash: :name, :length, :dir, :mtime, :mode, :qid, :dev
y_stathash:
            stz         cf
            stz         cf + 1
            LDAX        y_buf + SR_NAME                     ; (Its name)
            sta         hq
            stx         hq + 1
            ldy         #0
:
            lda         (hq),y
            beq         :+
            iny
            cpy         #32
            bcc         :-
:
            sty         hn
            stz         hn + 1
            jsr         string_make
            bcc         :+
            jmp         y_nomem
:
            LDAX        s_kname
            jsr         y_entry
            ldx         #3                                  ; (Its length: a directory's 0)
:
            lda         y_buf + SR_LENGTH,x
            sta         n_w32,x
            dex
            bpl         :-
            jsr         y_isdir
            beq         :+
            stz         n_w32
            stz         n_w32 + 1
            stz         n_w32 + 2
            stz         n_w32 + 3
:
            jsr         y_u32
            MOVW        hv, ex
            LDAX        s_klength
            jsr         y_entry
            stz         hv                                  ; (A directory?)
            stz         hv + 1
            jsr         y_isdir
            beq         :+
            lda         #<T_VAL
            sta         hv
:
            LDAX        s_kdir
            jsr         y_entry
            ldx         #3                                  ; (When it changed)
:
            lda         y_buf + SR_MTIME,x
            sta         n_w32,x
            dex
            bpl         :-
            jsr         y_u32
            MOVW        hv, ex
            LDAX        s_kmtime
            jsr         y_entry
            lda         y_buf + SR_MODE                     ; (Its mode)
            ldx         y_buf + SR_MODE + 1
            sta         n_w32
            stx         n_w32 + 1
            stz         n_w32 + 2
            stz         n_w32 + 3
            jsr         y_u32
            MOVW        hv, ex
            LDAX        s_kmode
            jsr         y_entry
            ldx         #3                                  ; (Its qid's path)
:
            lda         y_buf + SR_QPATH,x
            sta         n_w32,x
            dex
            bpl         :-
            jsr         y_u32
            MOVW        hv, ex
            LDAX        s_kqid
            jsr         y_entry
            lda         y_buf + SR_DEV                      ; (Its device's letter, a character)
            stz         hv + 1
            asl
            sta         hv
            rol         hv + 1
            lda         hv + 1
            ora         #>CHAR0
            sta         hv + 1
            LDAX        s_kdev
            jsr         y_entry
            FARN        4, h_fromlist
            bcc         :+
            jmp         y_nomem
:
            rts

; ---- Files and directories

; (read-file p): the file's bytes, a string
bi_read_file:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_sdone
            jsr         y_readfile
            bcc         y_sdone
            jmp         y_syserr0
y_sdone:
            clc
            rts

; ex = the file y_path's bytes, a string.  OUT: C = 1 and .A = the system's error (ex not set) if it can't be read
y_readfile:
            jsr         y_stat
            bcs         @rts
            jsr         y_isdir
            beq         :+
            lda         #E_ISDIR
            sec
            rts
:
            lda         #O_READ
            jsr         y_open
            bcs         @rts
            jsr         y_slurp
            pha
            lda         y_fd
            jsr         CLOSE
            pla
            cmp         #0
            bne         :+
            clc
            rts
:
            sec
@rts:
            rts

; (read-lines p): the file's lines (LF, or CR LF; a last one with no LF too), a list of strings
bi_read_lines:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_sdone
            jsr         y_readfile
            bcc         :+
            jmp         y_syserr0
:
            lda         ex                                  ; (The bytes, kept; y_i the place, y_n the length)
            ldx         ex + 1
            sta         y_v
            stx         y_v + 1
            jsr         y_rpush
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            sta         y_n
            stx         y_n + 1
            stz         y_i
            stz         y_i + 1
            stz         cf
            stz         cf + 1
@line:
            lda         y_i                                 ; (At the end: done)
            cmp         y_n
            lda         y_i + 1
            sbc         y_n + 1
            bcs         @done
            jsr         y_begin
            jsr         y_at                                ; (nb: its bytes from y_i)
@byte:
            lda         y_i
            cmp         y_n
            lda         y_i + 1
            sbc         y_n + 1
            bcs         @end
            lda         (nb)
            jsr         y_next
            cmp         #LF
            beq         @end
            cmp         #CR                                 ; (CR LF: the line's end)
            bne         @out
            lda         y_i
            cmp         y_n
            lda         y_i + 1
            sbc         y_n + 1
            bcs         @cr
            lda         (nb)
            cmp         #LF
            bne         @cr
            jsr         y_next
            bra         @end
@cr:
            lda         #CR
@out:
            jsr         out_byte
            bra         @byte
@end:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            lda         hv
            ldx         hv + 1
            jsr         y_append
            bra         @line
@done:
            dec         rsp
            MOVW        ex, cf
            clc
            rts

; nb = the address of y_v's (a string's) byte y_i, in the window
y_at:
            lda         y_v
            ldx         y_v + 1
            ldy         #1
            jsr         cell_get
            jsr         blob_data
            clc
            lda         hp
            adc         y_i
            sta         nb
            lda         hp + 1
            adc         y_i + 1
            sta         nb + 1
            rts

; y_i and nb on a byte.  Keeps .A
y_next:
            inc         y_i
            bne         :+
            inc         y_i + 1
:
            inc         nb
            bne         :+
            inc         nb + 1
:
            rts

; (write-file p x...), (append-file p x...): the values as print shows them, written to the file (made, or emptied
; first), or added at its end; NIL
bi_write_file:
            lda         #0
            bra         y_putfile
bi_append_file:
            lda         #1
y_putfile:
            sta         y_c
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_sdone2
            jsr         y_begin                             ; (The values: a string)
            lda         #1
            sta         y_n
@item:
            lda         y_n
            cmp         cn
            bcs         @made
            asl
            tay
            jsr         y_argy
            ldy         #1
            FARN        2, print_val
            inc         y_n
            bra         @item
@made:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            lda         hv
            ldx         hv + 1
            jsr         y_rpush
            lda         y_c
            bne         :+
            jsr         y_create
            bra         @opened
:
            jsr         y_append_to
@opened:
            bcc         :+
            dec         rsp                                 ; (The string, dropped)
            bra         @fail
:
            jsr         root_pop
            jsr         y_spill
            php
            pha
            lda         y_fd
            jsr         CLOSE
            pla
            plp
            bcs         @fail
            jmp         y_nil
@fail:
            jmp         y_syserr0
y_sdone2:
            clc
            rts

; (ls [p]): the names in the directory (the current one), sorted; a file's, its own
bi_ls:
            jsr         y_enter
            jsr         y_dirarg
            bcs         y_sdone2
            jsr         y_names
            bcc         :+
            jmp         y_syserrp
:
            MOVW        ex, hv
            clc
            rts

; The argument 0 a path in y_path, or "." if there's none.  OUT: C = 1 and ex = the error if it isn't one
y_dirarg:
            lda         cn
            bne         :+
            lda         #'.'
            sta         y_path
            stz         y_path + 1
            clc
            rts
:
            ldy         #0
            ldx         #0
            jmp         y_patharg

; (dir [p]): a stat hash for each name ls gives
bi_dir:
            jsr         y_enter
            jsr         y_dirarg
            bcs         y_sdone2
            jsr         y_names
            bcc         :+
            jmp         y_syserrp
:
            lda         hv                                  ; (The names, kept, walked: y_v)
            ldx         hv + 1
            sta         y_v
            stx         y_v + 1
            jsr         y_rpush
            ldx         #PATH_MAX                           ; (The directory's path: y_path2)
:
            lda         y_path,x
            sta         y_path2,x
            dex
            bpl         :-
            jsr         y_stat                              ; (A file: its own stat)
            jsr         y_isdir
            sta         y_any
            stz         cf
            stz         cf + 1
@name:
            lda         y_v
            ora         y_v + 1
            beq         @done
            lda         y_any
            beq         @stat
            jsr         y_join                              ; (y_path: the directory's path, / and the name)
            bcc         @stat
            lda         #E_NAMETOOLONG
            bra         @fail
@stat:
            jsr         y_stat
            bcc         :+
@fail:
            dec         rsp                                 ; (The names, dropped)
            jmp         y_syserrp
:
            lda         cf                                  ; (Its hash: cf and cr kept)
            ldx         cf + 1
            jsr         y_rpush
            lda         cr
            ldx         cr + 1
            jsr         y_rpush
            jsr         y_stathash
            jsr         root_pop
            sta         cr
            stx         cr + 1
            jsr         root_pop
            sta         cf
            stx         cf + 1
            lda         hv
            ldx         hv + 1
            jsr         y_append
            lda         y_v
            ldx         y_v + 1
            ldy         #1
            jsr         cell_get
            sta         y_v
            stx         y_v + 1
            bra         @name
@done:
            dec         rsp
            MOVW        ex, cf
            clc
            rts

; y_path = y_path2, "/" and the name that's y_v's first.  OUT: C = 1 if it's too long
y_join:
            ldy         #0
:
            lda         y_path2,y
            beq         :+
            sta         y_path,y
            iny
            bra         :-
:
            cpy         #0                                  ; ("/": no / more)
            beq         :+
            lda         y_path - 1,y
            cmp         #'/'
            beq         :+
            lda         #'/'
            sta         y_path,y
            iny
:
            sty         y_c
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            jsr         deref
            lda         (hp)
            sta         y_n
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            ldx         y_c
            ldy         #0
:
            cpy         y_n
            beq         :+
            cpx         #PATH_MAX
            bcs         @long
            lda         (hp),y
            sta         y_path,x
            inx
            iny
            bra         :-
:
            stz         y_path,x
            clc
            rts
@long:
            sec
            rts

; (stat p): the file's stat hash: :name, :length, :dir, :mtime (seconds since 2000), :mode, :qid, :dev
bi_stat:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_sdone3
            jsr         y_stat
            bcc         :+
            jmp         y_syserr0
:
            jsr         y_stathash
            MOVW        ex, hv
y_sdone3:
            clc
            rts

; (exists? p), (dir? p), (file? p): whether it's there; a directory; a file
bi_exists_p:
            lda         #0
            bra         y_test
bi_dir_p:
            lda         #1
            bra         y_test
bi_file_p:
            lda         #2
y_test:
            sta         y_c
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_sdone3
            jsr         y_stat
            bcc         :+
            jmp         y_nil
:
            lda         y_c
            beq         @yes
            jsr         y_isdir
            php
            lda         y_c
            cmp         #1
            beq         :+
            plp                                             ; (file?: not a directory)
            beq         @yes
            jmp         y_nil
:
            plp                                             ; (dir?)
            bne         @yes
            jmp         y_nil
@yes:
            jmp         y_t

; (mkdir p): the directory made; NIL
bi_mkdir:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_sdone3
            LDR         r0, y_path
            lda         #O_READ
            ldx         #DM_DIR
            jsr         CREATE
            bcs         :+
            jsr         CLOSE
            jmp         y_nil
:
            jmp         y_syserr0

; (remove p): the file (or the empty directory) taken away; NIL
bi_remove:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_sdone3
            LDR         r0, y_path
            jsr         REMOVE
            bcs         :+
            jmp         y_nil
:
            jmp         y_syserr0

; (rename p q): the file called q's name (in its own directory: WSTAT's, as mv's); NIL
bi_rename:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_sdone4
            ldy         #2
            ldx         #1
            jsr         y_patharg
            bcs         y_sdone4
            jsr         y_stat                              ; (p there)
            bcc         :+
            jmp         y_syserr0
:
            LDR         r0, y_path2                         ; (q not)
            LDR         r1, y_buf
            jsr         STAT
            bcs         :+
            lda         #E_EXIST
            jmp         y_syserr1
:
            ldx         #SR_SIZE - 1                        ; (Its new name, nothing else)
            lda         #$FF
:
            sta         y_buf,x
            dex
            bpl         :-
            ldx         #0                                  ; (q's last element)
            ldy         #0
@find:
            lda         y_path2,x
            beq         @base
            inx
            cmp         #'/'
            bne         @find
            txa
            tay
            bra         @find
@base:
            ldx         #0
:
            lda         y_path2,y
            sta         y_buf + SR_NAME,x
            beq         :+
            inx
            iny
            cpx         #31
            bcc         :-
            stz         y_buf + SR_NAME,x
:
            LDR         r0, y_path
            LDR         r1, y_buf
            jsr         WSTAT
            bcs         :+
            jmp         y_nil
:
            jmp         y_syserr0
y_sdone4:
            clc
            rts

; (copy-file p q): q made (or emptied) with p's bytes; NIL
bi_copy_file:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_sdone4
            ldy         #2
            ldx         #1
            jsr         y_patharg
            bcs         y_sdone4
            jsr         y_stat
            bcs         @fail
            jsr         y_isdir
            beq         :+
            lda         #E_ISDIR
            bra         @fail
:
            lda         #O_READ
            jsr         y_open
            bcs         @fail
            lda         y_fd
            sta         y_task                              ; (p's fd: y_task)
            ldx         #PATH_MAX
:
            lda         y_path2,x
            sta         y_path,x
            dex
            bpl         :-
            jsr         y_create
            bcs         @closep
@chunk:
            LDR         r0, y_buf
            LDR         r1, 256
            lda         y_task
            jsr         READ
            bcs         @both
            sta         r1
            stx         r1 + 1
            ora         r1 + 1
            beq         @end
            LDR         r0, y_buf
            lda         y_fd
            jsr         WRITE
            bcc         @chunk
@both:
            pha
            lda         y_fd
            jsr         CLOSE
            pla
@closep:
            pha
            lda         y_task
            jsr         CLOSE
            pla
@fail:
            jmp         y_syserr0
@end:
            lda         y_fd
            jsr         CLOSE
            lda         y_task
            jsr         CLOSE
            jmp         y_nil

; (cd [p]): the current directory p (/ if there's none); NIL
bi_cd:
            jsr         y_enter
            lda         cn
            bne         :+
            lda         #'/'
            sta         y_path
            stz         y_path + 1
            bra         @cd
:
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_sdone5
@cd:
            LDR         r0, y_path
            jsr         CHDIR
            bcs         :+
            jmp         y_nil
:
            ldx         cn
            bne         :+
            jmp         y_syserrp
:
            jmp         y_syserr0

; (cwd): the current directory's path
bi_cwd:
            jsr         y_enter
            LDR         r0, y_path
            jsr         GETCWD
            bcc         :+
            stz         y_name
            stz         y_name + 1
            jmp         y_syserr
:
            jsr         y_pathstr
            MOVW        ex, hv
y_sdone5:
            clc
            rts

; hv = y_path, a string
y_pathstr:
            LDHQ        y_path, 0
            ldy         #0
:
            lda         y_path,y
            beq         :+
            iny
            bne         :-
:
            sty         hn
            jsr         string_make
            bcc         :+
            jmp         y_nomem
:
            rts

; (glob pattern): the paths that match it (each element of it a pattern: * any run, ? any byte, [...] one of them,
; a-z a range, ~ first none of them), sorted
bi_glob:
            jsr         y_enter
            ldy         #0
            ldx         #1
            jsr         y_patharg                           ; (The pattern: y_path2)
            bcs         y_sdone5
            stz         y_pos                               ; (Its next element's first byte)
            stz         y_part
            lda         #<SNIL                              ; (The paths so far: one, nothing yet (SNIL: none at
            sta         et                                  ;   all is NIL))
            stz         et + 1
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         y_make2
            lda         hv
            ldx         hv + 1
            jsr         y_rpush                             ; (The paths: on the root stack)
@part:
            ldx         y_pos                               ; (The element: y_pat; any pattern in it: y_any)
            ldy         #0
            stz         y_any
:
            lda         y_path2,x
            beq         :+
            cmp         #'/'
            beq         :+
            sta         y_pat,y
            cmp         #'*'
            beq         @wild
            cmp         #'?'
            beq         @wild
            cmp         #'['
            bne         @plain
@wild:
            inc         y_any
@plain:
            inx
            iny
            bra         :-
:
            lda         #0
            sta         y_pat,y
            stx         y_pos
            stz         cf                                  ; (The paths after it: cf, cr)
            stz         cf + 1
            stz         cr
            stz         cr + 1
            jsr         y_top
@path:
            sta         y_v
            stx         y_v + 1
            ora         y_v + 1
            bne         :+
            jmp         @next
:
            lda         y_part                              ; (An absolute path's first element: "")
            bne         @notfirst
            lda         y_pat
            bne         @notfirst
            jsr         y_begin
            jsr         y_madepath
            jmp         @on
@notfirst:
            lda         y_any
            bne         @match
            jsr         y_begin                             ; (Not a pattern: the path and it)
            jsr         y_prefix
            LDAX        y_pat
            jsr         out_text
            jsr         y_madepath
            jmp         @on
@match:
            jsr         y_dirof                             ; (A pattern: the directory's names that match it)
            jsr         y_names
            bcs         @on                                 ; (No such directory: none)
            lda         hv
            ldx         hv + 1
            jsr         y_rpush
            lda         hv
            ldx         hv + 1
@name:
            sta         y_w
            stx         y_w + 1
            ora         y_w + 1
            beq         @names
            jsr         y_namebytes                         ; (Its name: y_name32)
            jsr         y_match
            bcc         :+
            jsr         y_begin
            jsr         y_prefix
            LDAX        y_name32
            jsr         out_text
            jsr         y_madepath
:
            lda         y_w
            ldx         y_w + 1
            ldy         #1
            jsr         cell_get
            bra         @name
@names:
            dec         rsp
@on:
            lda         y_v
            ldx         y_v + 1
            ldy         #1
            jsr         cell_get
            jmp         @path
@next:
            ldx         rsp                                 ; (The paths: those after it)
            dex
            lda         cf
            sta         rs_lo,x
            lda         cf + 1
            sta         rs_hi,x
            inc         y_part
            ldx         y_pos
            lda         y_path2,x
            beq         @last
            inc         y_pos
            jmp         @part
@last:
            stz         cf                                  ; (Those there, sorted)
            stz         cf + 1
            lda         #0
            tax
            jsr         y_rpush
            lda         rsp
            sta         y_lroot
            dec         y_lroot
            ldx         rsp
            dex
            dex
            lda         rs_lo,x
            ldy         rs_hi,x
@found:
            sta         y_v
            sty         y_v + 1
            ora         y_v + 1
            beq         @sorted
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            jsr         y_pathcopy
            jsr         y_stat
            bcs         :+
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            jsr         y_insval
:
            lda         y_v
            ldx         y_v + 1
            ldy         #1
            jsr         cell_get
            phx
            ply
            bra         @found
@sorted:
            jsr         root_pop
            sta         ex
            stx         ex + 1
            dec         rsp
            clc
            rts

; .A, .X = the paths so far (the root stack's top)
y_top:
            ldx         rsp
            dex
            lda         rs_lo,x
            pha
            lda         rs_hi,x
            tax
            pla
            rts

; The path y_v's first (a string; SNIL: none yet, the pattern's first element) out, and a / after it
y_prefix:
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            cmp         #<SNIL
            bne         :+
            cpx         #0
            beq         @rts
:
            ldy         #1
            FARN        2, print_val
            lda         #'/'
            jsr         out_byte
@rts:
            rts

; The capture begun made a string, put at the end of the paths after it (cf, cr)
y_madepath:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            lda         hv
            ldx         hv + 1
            jmp         y_append

; y_path = the directory of the path y_v's first: "." for none yet (SNIL), "/" for "" (the root), else it
y_dirof:
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            cmp         #<SNIL
            bne         :+
            cpx         #0
            bne         :+
            lda         #'.'
            sta         y_path
            stz         y_path + 1
            rts
:
            jsr         y_pathcopy
            lda         y_path
            bne         :+
            lda         #'/'
            sta         y_path
            stz         y_path + 1
:
            rts

; y_path = the string .A, .X (SNIL: ""), a 0 after it (PATH_MAX bytes at most)
y_pathcopy:
            cmp         #<SNIL
            bne         :+
            cpx         #0
            bne         :+
            stz         y_path
            rts
:
            jsr         deref
            lda         (hp)
            sta         y_n
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            ldy         #0
:
            cpy         y_n
            beq         :+
            cpy         #PATH_MAX
            bcs         :+
            lda         (hp),y
            sta         y_path,y
            iny
            bra         :-
:
            lda         #0
            sta         y_path,y
            rts

; y_name32 = the name that's y_w's first (a string), a 0 after it
y_namebytes:
            lda         y_w
            ldx         y_w + 1
            ldy         #0
            jsr         cell_get
            jsr         deref
            lda         (hp)
            sta         y_n
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            ldy         #0
:
            cpy         y_n
            beq         :+
            cpy         #31
            bcs         :+
            lda         (hp),y
            sta         y_name32,y
            iny
            bra         :-
:
            lda         #0
            sta         y_name32,y
            rts

; The string .A, .X put in order in y_names' list (y_lroot's)
y_insval:
            sta         hv
            stx         hv + 1
            jmp         y_insstr

; Does y_name32 match the pattern y_pat (rc's)?  OUT: C = 1 if it does
y_match:
            stz         y_pi
            stz         y_si
            lda         #$FF
            sta         y_star
@step:
            ldx         y_si
            lda         y_name32,x
            beq         @rest
            ldy         y_pi
            lda         y_pat,y
            cmp         #'*'
            bne         :+
            sty         y_star
            inc         y_pi
            stx         y_mark
            bra         @step
:
            jsr         y_one
            bcc         @back
            inc         y_si
            bra         @step
@back:
            lda         y_star                              ; (No * before: no)
            cmp         #$FF
            beq         @no
            inc         a
            sta         y_pi
            inc         y_mark
            lda         y_mark
            sta         y_si
            bra         @step
@rest:
            ldy         y_pi                                ; (The name's end: only *s left)
:
            lda         y_pat,y
            beq         @yes
            cmp         #'*'
            bne         @no
            iny
            bra         :-
@yes:
            sec
            rts
@no:
            clc
            rts

; Does the pattern's next (y_pi: moved past it) match the byte y_name32,y_si?  OUT: C = 1 if it does
y_one:
            ldy         y_pi
            lda         y_pat,y
            beq         @no
            cmp         #'?'
            beq         @any
            cmp         #'['
            beq         @set
@byte:
            iny
            sty         y_pi
            ldx         y_si
            cmp         y_name32,x
            bne         @no
            sec
            rts
@any:
            iny
            sty         y_pi
            sec
            rts
@no:
            clc
            rts
@set:
            iny                                             ; (A set: ~ first, none of them)
            stz         y_c
            lda         y_pat,y
            cmp         #'~'
            bne         :+
            inc         y_c
            iny
:
            stz         y_cnt                               ; (Found: y_cnt)
            ldx         y_si
@item:
            lda         y_pat,y
            beq         @noclose
            cmp         #']'
            beq         @close
            lda         y_pat + 1,y                         ; (a-z)
            cmp         #'-'
            bne         @single
            lda         y_pat + 2,y
            beq         @single
            cmp         #']'
            beq         @single
            lda         y_name32,x
            cmp         y_pat,y
            bcc         :+
            cmp         y_pat + 2,y
            beq         @in
            bcs         :+
@in:
            inc         y_cnt
:
            iny
            iny
            iny
            bra         @item
@single:
            lda         y_pat,y
            cmp         y_name32,x
            bne         :+
            inc         y_cnt
:
            iny
            bra         @item
@close:
            iny
            sty         y_pi
            lda         y_cnt
            beq         :+
            lda         #1
:
            eor         y_c                                 ; (Found, unless ~)
            lsr
            rts
@noclose:
            ldy         y_pi                                ; (No ]: a [ as itself)
            lda         #'['
            bra         @byte

; ---- Programs and the shell

; (run prog x...): the program run (a bare name /bin's), the values its arguments (as print shows them); its exit code
bi_run:
            jsr         y_enter
            jsr         y_prog
            bcs         y_pdone
            lda         #1                                  ; (Its arguments: from 1)
            jsr         y_args
            bcs         y_pdone
            jsr         y_spawn
            bcs         y_perr
            jmp         y_wait
y_perr:
            jmp         y_syserr0
y_pdone:
            clc
            rts

; (spawn prog x...): the program started, not waited for: its task
bi_spawn:
            jsr         y_enter
            jsr         y_prog
            bcs         y_pdone
            lda         #1
            jsr         y_args
            bcs         y_pdone
            jsr         y_spawn
            bcs         y_perr
            lda         y_task
            ldx         #0
            jmp         y_fix

; y_path = the program's path: the argument 0 (a bare name: /bin/ and it).  OUT: C = 1 and ex = the error
y_prog:
            ldy         #0
            ldx         #1
            jsr         y_patharg
            bcs         @rts
            ldx         #0                                  ; (A / in it: as it is)
:
            lda         y_path2,x
            beq         @bare
            cmp         #'/'
            beq         @path
            inx
            bra         :-
@bare:
            ldx         #4
:
            lda         s_bin,x
            sta         y_path,x
            dex
            bpl         :-
            ldx         #0
            ldy         #5
:
            lda         y_path2,x
            sta         y_path,y
            beq         @done
            inx
            iny
            cpy         #PATH_MAX
            bcc         :-
            lda         #E_NAMETOOLONG
            jsr         y_syserr0
            sec
            rts
@path:
            ldx         #PATH_MAX
:
            lda         y_path2,x
            sta         y_path,x
            dex
            bpl         :-
@done:
            clc
@rts:
            rts

; y_buf = the arguments from .A on, as print shows them, each with a 0 after it, and an empty one after them all
; (ARGS_MAX bytes at most).  OUT: C = 1 and ex = the error ("too big") if they don't fit
y_args:
            sta         y_n
            jsr         y_begin
@arg:
            lda         y_n
            cmp         cn
            bcs         @end
            asl
            tay
            jsr         y_argy
            ldy         #1
            FARN        2, print_val
            lda         #0
            jsr         out_byte
            inc         y_n
            bra         @arg
@end:
            lda         #0
            jsr         out_byte
            jsr         y_capbuf
            bcc         :+
            lda         #E_TOOBIG
            stz         y_name
            stz         y_name + 1
            jsr         y_syserr
            sec
:
            rts

; The capture begun (y_begin's) given up, its bytes copied to y_buf (ARGS_MAX at most).  OUT: C = 1 if there were more
y_capbuf:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            lda         hv
            ldx         hv + 1
            jsr         deref
            lda         (hp)
            sta         y_n
            ldy         #1
            lda         (hp),y
            bne         @long
            lda         y_n
            cmp         #ARGS_MAX + 1
            bcs         @long
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            ldy         #0
:
            cpy         y_n
            beq         :+
            lda         (hp),y
            sta         y_buf,y
            iny
            bra         :-
:
            clc
            rts
@long:
            sec
            rts

; y_path's program started, its arguments y_buf, its fds .X's map's (0: this task's 0, 1 and 2).  OUT: C = 0, y_task
; its task; C = 1, .A = the system's error
y_spawn:
            ldx         #0
y_spawnmap:
            jsr         out_flush                           ; (What's out first: its output after it)
            LDR         r0, y_path
            LDR         r1, y_buf
            LDR         r2, y_map
            lda         #0
            cpx         #0
            beq         :+
            lda         #SPAWN_FDMAP
:
            jsr         SPAWN
            bcs         :+
            sta         y_task
:
            rts

; ex = y_task's exit code, when it ends: its message's number if it has one (rc's exit status: exit 3 is code 1,
; the message "3"), else its code
y_wait:
            LDR         r0, y_buf
            lda         y_task
            jsr         WAIT
            bcc         :+
            jmp         y_syserr0
:
            stx         y_c
            stz         y_n                                 ; (Its message's digits: y_n)
            stz         y_n + 1
            ldy         #0
            lda         y_buf
            beq         @code
@digit:
            lda         y_buf,y
            beq         @number
            sec
            sbc         #'0'
            cmp         #10
            bcs         @code
            pha
            lda         y_n + 1                             ; (Past 1000: not one)
            cmp         #>1000
            bcs         @big
            lda         y_n                                 ; (y_n * 10 + it)
            asl
            sta         y_i
            lda         y_n + 1
            rol
            sta         y_i + 1
            asl         y_n
            rol         y_n + 1
            asl         y_n
            rol         y_n + 1
            asl         y_n
            rol         y_n + 1
            clc
            lda         y_n
            adc         y_i
            sta         y_n
            lda         y_n + 1
            adc         y_i + 1
            sta         y_n + 1
            pla
            clc
            adc         y_n
            sta         y_n
            bcc         :+
            inc         y_n + 1
:
            iny
            bra         @digit
@big:
            pla
@code:
            lda         y_c
            ldx         #0
            jmp         y_fix
@number:
            lda         y_n
            ldx         y_n + 1
            jmp         y_fix

; (wait task): the child task's exit code, when it ends
bi_wait:
            jsr         y_enter
            jsr         y_taskarg
            bcs         y_pdone2
            lda         y_task                              ; (Not a task: "30000: no such child")
            cmp         #$FF
            bne         :+
            lda         #E_CHILD
            jmp         y_syserr0
:
            jmp         y_wait

; (kill task): the task sent the kill note; NIL
bi_kill:
            jsr         y_enter
            jsr         y_taskarg
            bcs         y_pdone2
            lda         y_task
            cmp         #$FF
            bne         :+
            lda         #E_SRCH
            jmp         y_syserr0
:
            ldx         #NOTE_KILL
            jsr         NOTE
            bcc         :+
            jmp         y_syserr0
:
            jmp         y_nil
y_pdone2:
            clc
            rts

; y_task = the argument 0, a task (0-15; $FF: a number that isn't one).  OUT: C = 1 and ex = the error ("A task must
; be an integer") if it isn't an integer
y_taskarg:
            jsr         y_arg0
            FARN        3, n_to_u32
            bcc         :+
            jsr         y_arg0
            FARN        3, n_kind
            beq         @neg                                ; (A negative integer: no such task)
            LDAX        s_task
            jsr         y_inval
            sec
            rts
:
            lda         n_w32 + 1
            ora         n_w32 + 2
            ora         n_w32 + 3
            bne         @neg
            lda         n_w32
            cmp         #16
            bcs         @neg
            sta         y_task
            clc
            rts
@neg:
            lda         #$FF
            sta         y_task
            clc
            rts

; (pid): this task's number
bi_pid:
            jsr         GETPID
            ldx         #0
            jmp         y_fix

; (sh line [input]), (sh-out line [input]): the line run by rc (rc -c), its input the text (or this one's); its exit
; code, or (sh-out) its output, a string
bi_sh:
            lda         #0
            bra         y_sh
bi_sh_out:
            lda         #1
y_sh:
            sta         y_c
            jsr         y_enter
            ldx         #7                                  ; (rc: /bin/rc)
:
            lda         s_rc,x
            sta         y_path,x
            dex
            bpl         :-
            jsr         y_begin                             ; (Its arguments: -c and the line)
            LDAX        s_dashc
            jsr         out_text
            lda         #0
            jsr         out_byte
            jsr         y_arg0
            ldy         #1
            FARN        2, print_val
            lda         #0
            jsr         out_byte
            jsr         out_byte
            jsr         y_capbuf
            bcc         :+
            lda         #E_TOOBIG
            jmp         y_syserr0
:
            lda         #3                                  ; (Its fds: this one's, or the pipes')
            sta         y_map
            stz         y_map + 1
            lda         #1
            sta         y_map + 2
            lda         #2
            sta         y_map + 3
            lda         #$FF
            sta         y_pipe
            sta         y_pipe + 1
            sta         y_pipe + 2
            sta         y_pipe + 3
            lda         cn                                  ; (Its input: a pipe, the text written to it)
            cmp         #2
            bcc         @output
            jsr         PIPE
            bcc         :+
            jmp         y_syserr0
:
            sta         y_pipe
            stx         y_pipe + 1
            sta         y_map + 1
@output:
            lda         y_c                                 ; (Its output, sh-out's: a pipe)
            beq         @start
            jsr         PIPE
            bcc         :+
            jsr         y_closepipes
            jmp         y_syserr0
:
            sta         y_pipe + 2
            stx         y_pipe + 3
            stx         y_map + 2
@start:
            ldx         #1
            jsr         y_spawnmap
            bcc         :+
            jsr         y_closepipes
            jmp         y_syserr0
:
            lda         y_pipe                              ; (Its ends: closed here)
            jsr         y_close
            lda         y_pipe + 3
            jsr         y_close
            lda         y_pipe + 1                          ; (Its input written, then its end)
            cmp         #$FF
            beq         @read
            sta         y_fd
            jsr         y_arg1
            jsr         y_textof
            jsr         y_spill
            lda         y_pipe + 1
            jsr         y_close
@read:
            lda         y_c
            beq         @wait
            lda         y_pipe + 2                          ; (sh-out: its output, to its end)
            sta         y_fd
            jsr         y_slurp
            lda         y_pipe + 2
            jsr         y_close
            lda         ex
            ldx         ex + 1
            jsr         y_rpush
            stz         r0
            stz         r0 + 1
            lda         y_task
            jsr         WAIT
            jsr         root_pop
            sta         ex
            stx         ex + 1
            clc
            rts
@wait:
            jmp         y_wait

; The fd .A closed, unless it's $FF
y_close:
            cmp         #$FF
            beq         :+
            jsr         CLOSE
:
            rts

; The pipes' ends closed
y_closepipes:
            pha
            ldx         #3
:
            lda         y_pipe,x
            phx
            jsr         y_close
            plx
            dex
            bpl         :-
            pla
            rts

; .A, .X = the value .A, .X as print shows it, a string
y_textof:
            pha
            phx
            jsr         kind_of
            cmp         #PK_STRING
            bne         :+
            plx
            pla
            rts
:
            jsr         y_begin
            plx
            pla
            ldy         #1
            FARN        2, print_val
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            lda         hv
            ldx         hv + 1
            rts

; ---- The environment

; (env [name]): the variable's value (a string; several words, a list of them), NIL if there's none; or, with no
; name, a hash of them all
bi_env:
            jsr         y_enter
            lda         cn
            beq         @all
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcc         @lb414
            jmp         y_edone
@lb414:
            jsr         y_envval
            bcs         @lb413
            jmp         y_edone
@lb413:
            jmp         y_syserr0
@all:
            stz         cf
            stz         cf + 1
            stz         y_c                                 ; (Variable y_c)
@var:
            LDR         r0, y_path
            ldx         y_c
            lda         #$FF
            jsr         ENV_NAME
            bcs         @made
            lda         cf
            ldx         cf + 1
            jsr         y_rpush
            lda         cr
            ldx         cr + 1
            jsr         y_rpush
            jsr         y_pathstr                           ; ({name value})
            lda         hv
            ldx         hv + 1
            jsr         y_rpush
            jsr         y_envval
            bcc         :+
            jmp         y_syserrp
:
            MOVW        eu, ex
            ldx         rsp                                 ; (The name: kept while the pair's made)
            dex
            lda         rs_lo,x
            sta         et
            lda         rs_hi,x
            sta         et + 1
            jsr         y_pair
            dec         rsp
            jsr         root_pop
            sta         cr
            stx         cr + 1
            jsr         root_pop
            sta         cf
            stx         cf + 1
            lda         hv
            ldx         hv + 1
            jsr         y_append
            inc         y_c
            bra         @var
@made:
            FARN        4, h_fromlist
            bcc         :+
            jmp         y_nomem
:
            MOVW        ex, hv
y_edone:
            clc
            rts

; ex = the variable y_path's value: a string (one word), a list of strings (several), NIL (none).  OUT: C = 1, .A =
; the system's error (ex not set) if it can't be read
y_envval:
            jsr         y_begin                             ; (Its bytes: captured; y_i its offset)
            stz         y_i
            stz         y_i + 1
@part:
            LDR         r0, y_path
            LDR         r1, y_buf
            LDR         r2, 256
            lda         y_i
            sta         r3
            lda         y_i + 1
            sta         r3 + 1
            lda         #$FF
            jsr         ENV_GET
            bcs         @fail
            sta         y_n
            stx         y_n + 1
            ora         y_n + 1
            beq         @end
            ldx         #0
:
            lda         y_buf,x
            jsr         out_byte
            inx
            beq         :+
            cpx         y_n
            bne         :-
            lda         y_n + 1
            bne         :-
:
            txa                                             ; (On past them: 256 at most each time)
            bne         :+
            inc         y_i + 1
            bra         @part
:
            clc
            adc         y_i
            sta         y_i
            bcc         @part
            inc         y_i + 1
            bra         @part
@fail:
            pha
            lda         y_start
            ldx         y_start + 1
            jsr         cap_drop
            pla
            cmp         #E_NOENT                            ; (None: NIL)
            bne         :+
            jmp         y_nil
:
            sec
            rts
@end:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            MOVW        y_v, hv                             ; (Its words: each up to a 0)
            lda         hv
            ldx         hv + 1
            jsr         y_rpush
            lda         cf
            ldx         cf + 1
            jsr         y_rpush
            lda         cr
            ldx         cr + 1
            jsr         y_rpush
            stz         cf
            stz         cf + 1
            ldy         #0
            lda         y_v
            ldx         y_v + 1
            jsr         cell_get
            sta         y_n
            stx         y_n + 1
            stz         y_i
            stz         y_i + 1
            stz         y_cnt                               ; (Words: 2 is several)
@word:
            lda         y_i
            cmp         y_n
            lda         y_i + 1
            sbc         y_n + 1
            bcs         @words
            jsr         y_begin
            jsr         y_at
@wbyte:
            lda         y_i
            cmp         y_n
            lda         y_i + 1
            sbc         y_n + 1
            bcs         @wend
            lda         (nb)
            jsr         y_next
            cmp         #0
            beq         @wend
            jsr         out_byte
            bra         @wbyte
@wend:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            lda         hv
            ldx         hv + 1
            jsr         y_append
            inc         y_cnt
            bra         @word
@words:
            lda         y_cnt                               ; (One: the string; several: the list; none: "")
            cmp         #1
            bne         :+
            lda         cf
            ldx         cf + 1
            ldy         #0
            jsr         cell_get
            sta         ex
            stx         ex + 1
            bra         @back
:
            MOVW        ex, cf
            lda         y_cnt
            bne         @back
            LDHQ        y_buf, 0
            jsr         string_make
            bcc         :+
            jmp         y_nomem
:
            MOVW        ex, hv
@back:
            jsr         root_pop
            sta         cr
            stx         cr + 1
            jsr         root_pop
            sta         cf
            stx         cf + 1
            dec         rsp
            clc
            rts

; (setenv name value): the variable set (a list: its words, rc's); NIL
bi_setenv:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcc         @lb412
            jmp         y_edone2
@lb412:
            jsr         y_begin                             ; (Its bytes: each word and a 0)
            jsr         y_arg1
            jsr         q_conses
            bcs         @one
@word:
            sta         y_v
            stx         y_v + 1
            ora         y_v + 1
            beq         @made
            lda         y_v
            ldx         y_v + 1
            ldy         #0
            jsr         cell_get
            ldy         #1
            FARN        2, print_val
            lda         #0
            jsr         out_byte
            lda         y_v
            ldx         y_v + 1
            ldy         #1
            jsr         cell_get
            bra         @word
@one:
            jsr         y_arg1
            ldy         #1
            FARN        2, print_val
            lda         #0
            jsr         out_byte
@made:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            lda         hv                                  ; (Its bytes, 256 at a time, through y_buf)
            ldx         hv + 1
            sta         y_v
            stx         y_v + 1
            jsr         deref
            lda         (hp)
            sta         y_n
            ldy         #1
            lda         (hp),y
            sta         y_n + 1
            stz         y_i
            stz         y_i + 1
@part:
            jsr         y_at                                ; (nb: the bytes from y_i)
            ldx         #0
:
            lda         y_i
            cmp         y_n
            lda         y_i + 1
            sbc         y_n + 1
            bcs         :+
            lda         (nb)
            sta         y_buf,x
            jsr         y_next
            inx
            bne         :-
:
            stx         r2                                  ; (How many: 256 if .X went round)
            stz         r2 + 1
            txa
            bne         :+
            lda         y_i
            ora         y_i + 1
            beq         :+
            inc         r2 + 1
:
            sec                                             ; (Where they go: y_i less them)
            lda         y_i
            sbc         r2
            sta         r3
            lda         y_i + 1
            sbc         r2 + 1
            sta         r3 + 1
            LDR         r0, y_path
            LDR         r1, y_buf
            lda         #$FF
            jsr         ENV_PUT
            bcc         :+
            jmp         y_syserr0
:
            lda         y_i                                 ; (More: on)
            cmp         y_n
            lda         y_i + 1
            sbc         y_n + 1
            bcc         @part
            jmp         y_nil
y_edone2:
            clc
            rts

; (unsetenv name): the variable taken away; NIL
bi_unsetenv:
            jsr         y_enter
            ldy         #0
            ldx         #0
            jsr         y_patharg
            bcs         y_edone2
            LDR         r0, y_path
            lda         #$FF
            jsr         ENV_DEL
            bcc         :+
            cmp         #E_NOENT
            beq         :+
            jmp         y_syserr0
:
            jmp         y_nil

; ---- The clock

; (time): the clock, seconds since 2000-01-01 00:00:00
bi_time:
            jsr         y_enter
            jsr         TIME
            jmp         y_place

; (ticks): the tick count (TICK_HZ a second; 0-32767, it wraps)
bi_ticks:
            jsr         y_enter
            jsr         TICKS
            sta         n_w32
            txa
            and         #$7F
            sta         n_w32 + 1
            stz         n_w32 + 2
            stz         n_w32 + 3
            jmp         y_u32

; (tick-rate): TICK_HZ
bi_tick_rate:
            lda         #TICK_HZ
            ldx         #0
            jmp         y_fix

; (sleep s): seconds slept (a fraction too: (sleep 1/10)); NIL (a note ends it early: the error :intr)
bi_sleep:
            jsr         y_enter
            jsr         y_arg0
            FARN        3, n_ticks
            bcc         :+
            LDAX        s_sleep
            jmp         y_error
:
@more:
            lda         n_w32 + 1                           ; (32767 ticks at most at a time)
            ora         n_w32 + 2
            ora         n_w32 + 3
            bne         @long
            lda         n_w32
            ldx         #0
            stz         n_w32
            bra         @sleep
@long:
            lda         n_w32 + 1
            and         #$80
            ora         n_w32 + 2
            ora         n_w32 + 3
            bne         @max
            lda         n_w32
            ldx         n_w32 + 1
            stz         n_w32
            stz         n_w32 + 1
            bra         @sleep
@max:
            sec                                             ; (32767 now, the rest after)
            lda         n_w32
            sbc         #$FF
            sta         n_w32
            lda         n_w32 + 1
            sbc         #$7F
            sta         n_w32 + 1
            lda         n_w32 + 2
            sbc         #0
            sta         n_w32 + 2
            lda         n_w32 + 3
            sbc         #0
            sta         n_w32 + 3
            lda         #$FF
            ldx         #$7F
@sleep:
            cmp         #0
            bne         :+
            cpx         #0
            beq         @slept
:
            jsr         SLEEP
            bcc         @slept
            stz         y_name
            stz         y_name + 1
            jmp         y_syserr
@slept:
            lda         n_w32
            ora         n_w32 + 1
            ora         n_w32 + 2
            ora         n_w32 + 3
            bne         @more
            jmp         y_nil

; (date [t]): the time t (seconds since 2000; the clock's now) as "yyyy-mm-dd hh:mm:ss"
bi_date:
            jsr         y_enter
            jsr         y_timearg
            bcs         y_cdone
            jsr         y_parts
            jsr         y_begin
            lda         y_date                              ; (Its year, 4 digits)
            ldx         y_date + 1
            jsr         out_dec
            lda         #'-'
            ldx         #2
            jsr         y_two
            lda         #'-'
            ldx         #3
            jsr         y_two
            lda         #' '
            ldx         #4
            jsr         y_two
            lda         #':'
            ldx         #5
            jsr         y_two
            lda         #':'
            ldx         #6
            jsr         y_two
            jmp         y_end_str
y_cdone:
            clc
            rts

; .A out, then y_date's part .X in two digits
y_two:
            jsr         out_byte
            lda         y_date,x
            ldy         #'0' - 1
:
            iny
            sec
            sbc         #10
            bcs         :-
            adc         #10 + '0'
            pha
            tya
            jsr         out_byte
            pla
            jmp         out_byte

; The capture begun ended: ex the string; C = 0
y_end_str:
            lda         y_start
            ldx         y_start + 1
            jsr         cap_end
            bcc         :+
            jmp         y_nomem
:
            MOVW        ex, hv
            clc
            rts

; (date-parts [t]): a hash of the time's :year, :month, :day, :hour, :minute, :second, :weekday (0 Sunday)
bi_date_parts:
            jsr         y_enter
            jsr         y_timearg
            bcs         y_cdone
            jsr         y_parts
            stz         cf
            stz         cf + 1
            lda         y_date                              ; (Its year)
            ldx         y_date + 1
            jsr         fix_ax
            sta         hv
            stx         hv + 1
            LDAX        s_kyear
            jsr         y_entry
            ldx         #2
@part:
            phx
            lda         y_date,x
            ldx         #0
            jsr         fix_ax
            sta         hv
            stx         hv + 1
            plx
            phx
            lda         part_lo - 2,x
            pha
            lda         part_hi - 2,x
            tax
            pla
            jsr         y_entry
            plx
            inx
            cpx         #8
            bcc         @part
            FARN        4, h_fromlist
            bcc         :+
            jmp         y_nomem
:
            MOVW        ex, hv
            clc
            rts

; n_w32 = the argument 0, a time (an integer: seconds since 2000), or the clock's now if there's none.  OUT: C = 1
; and ex = the error ("a time must be an integer") if it isn't one
y_timearg:
            lda         cn
            bne         :+
            jsr         TIME
            ldx         #3
@now:
            lda         r0,x
            sta         n_w32,x
            dex
            bpl         @now
            clc
            rts
:
            jsr         y_arg0
            FARN        3, n_to_u32
            bcc         :+
            LDAX        s_atime
            jsr         y_inval
            sec
:
            rts

; y_date = the parts of the time n_w32: year (2 bytes), month, day, hour, minute, second, weekday
y_parts:
            lda         #60
            jsr         y_div8
            sta         y_date + 6                          ; (Its second ...
            lda         #60
            jsr         y_div8
            sta         y_date + 5                          ;   minute ...
            lda         #24
            jsr         y_div8
            sta         y_date + 4                          ;   hour ...
            lda         n_w32                               ;   and day: 2000-01-01 0 (a Saturday)
            sta         y_days
            lda         n_w32 + 1
            sta         y_days + 1
            lda         #7
            jsr         y_div8                              ; (Its weekday: (days + 6) mod 7)
            clc
            adc         #6
            cmp         #7
            bcc         :+
            sbc         #7
:
            sta         y_date + 7
            lda         #<2000
            sta         y_date
            lda         #>2000
            sta         y_date + 1
@year:
            jsr         y_yearlen                           ; (Its year: years taken away)
            lda         y_days
            cmp         y_n
            lda         y_days + 1
            sbc         y_n + 1
            bcc         @month
            sta         y_days + 1
            lda         y_days
            sec
            sbc         y_n
            sta         y_days
            inc         y_date
            bne         @year
            inc         y_date + 1
            bra         @year
@month:
            lda         #1
            sta         y_date + 2
@mon:
            jsr         y_monlen                            ; (Its month: months taken away)
            lda         y_days + 1
            bne         :+
            lda         y_days
            cmp         y_n
            bcc         @day
:
            sec
            lda         y_days
            sbc         y_n
            sta         y_days
            lda         y_days + 1
            sbc         #0
            sta         y_days + 1
            inc         y_date + 2
            bra         @mon
@day:
            inc         a
            sta         y_date + 3
            rts

; n_w32 = n_w32 / .A (a byte), .A = the remainder
y_div8:
            sta         y_c
            lda         #0
            ldx         #3
@byte:
            ldy         #8
            pha
            lda         n_w32,x
            sta         y_cnt
            pla
@bit:
            asl         y_cnt
            rol
            cmp         y_c
            bcc         :+
            sbc         y_c
            inc         y_cnt
:
            dey
            bne         @bit
            pha
            lda         y_cnt
            sta         n_w32,x
            pla
            dex
            bpl         @byte
            rts

; y_n = the days in the year y_date (365, 366 a leap year's)
y_yearlen:
            lda         #<365
            sta         y_n
            lda         #>365
            sta         y_n + 1
            jsr         y_leap
            bne         :+
            inc         y_n
:
            rts

; Is y_date's year a leap year?  OUT: Z = 1 if it is
y_leap:
            lda         y_date
            and         #3
            bne         @rts
            lda         y_date + 1                          ; (2100, 2200, 2300: not; 2000, 2400: so)
            cmp         #>2100
            bne         :+
            lda         y_date
            cmp         #<2100
            beq         @no
:
            lda         y_date + 1
            cmp         #>2200
            bne         :+
            lda         y_date
            cmp         #<2200
            beq         @no
:
            lda         y_date + 1
            cmp         #>2300
            bne         :+
            lda         y_date
            cmp         #<2300
            beq         @no
:
            lda         #0
@rts:
            rts
@no:
            lda         #1
            rts

; y_n = the days in the month y_date + 2 of the year y_date
y_monlen:
            ldx         y_date + 2
            lda         mon_days - 1,x
            sta         y_n
            stz         y_n + 1
            cpx         #2
            bne         :+
            jsr         y_leap
            bne         :+
            inc         y_n
:
            rts

; (seconds-of year month day [hour minute second]): the seconds since 2000 of that time
bi_seconds_of:
            jsr         y_enter
            ldx         #0                                  ; (Its parts: y_date's)
@arg:
            phx
            cpx         cn
            bcc         :+
            lda         #0                                  ; (hour, minute, second: 0 if not given)
            bra         @set
:
            txa
            asl
            tay
            jsr         y_argy
            FARN        3, n_to_u32
            bcc         :+
            plx
            LDAX        s_apart
            jmp         y_inval
:
            lda         n_w32 + 2
            ora         n_w32 + 3
            beq         @lb411
            jmp         @bad
@lb411:
            plx
            phx
            cpx         #0
            bne         :+
            lda         n_w32                               ; (The year: 2000-2135)
            sta         y_date
            lda         n_w32 + 1
            sta         y_date + 1
            plx
            ldx         #1
            bra         @arg
:
            lda         n_w32 + 1
            beq         @lb410
            jmp         @bad
@lb410:
            lda         n_w32
@set:
            plx
            sta         y_date + 1,x
            inx
            cpx         #6
            bcc         @arg
            lda         y_date + 1                          ; (In range?)
            cmp         #>2000
            bcc         @inval
            bne         :+
            lda         y_date
            cmp         #<2000
            bcc         @inval
:
            lda         y_date + 1
            cmp         #>2136
            bcc         :+
            bne         @inval
            lda         y_date
            cmp         #<2136
            bcs         @inval
:
            lda         y_date + 2
            beq         @inval
            cmp         #13
            bcs         @inval
            jsr         y_monlen
            lda         y_date + 3
            beq         @inval
            cmp         y_n
            beq         :+
            bcs         @inval
:
            lda         y_date + 4
            cmp         #24
            bcs         @inval
            lda         y_date + 5
            cmp         #60
            bcs         @inval
            lda         y_date + 6
            cmp         #60
            bcs         @inval
            jsr         y_daysof                            ; (Its days, then seconds)
            lda         y_days
            sta         n_w32
            lda         y_days + 1
            sta         n_w32 + 1
            stz         n_w32 + 2
            stz         n_w32 + 3
            lda         #24
            ldx         y_date + 4
            jsr         y_muladd
            lda         #60
            ldx         y_date + 5
            jsr         y_muladd
            lda         #60
            ldx         y_date + 6
            jsr         y_muladd
            jmp         y_u32
@bad:
            plx
@inval:
            lda         #E_INVAL
            jmp         y_syserr0

; y_days = the days from 2000-01-01 to y_date's year, month and day
y_daysof:
            lda         y_date                              ; (Its year, kept: years counted from 2000)
            pha
            lda         y_date + 1
            pha
            stz         y_days
            stz         y_days + 1
            lda         #<2000
            sta         y_date
            lda         #>2000
            sta         y_date + 1
@year:
            pla
            tax
            pla
            pha
            phx
            cmp         y_date
            bne         :+
            cpx         y_date + 1
            beq         @month
:
            jsr         y_yearlen
            jsr         y_adddays
            inc         y_date
            bne         @year
            inc         y_date + 1
            bra         @year
@month:
            pla
            sta         y_date + 1
            pla
            sta         y_date
            lda         y_date + 2
            pha
            lda         #1
            sta         y_date + 2
@mon:
            pla
            pha
            cmp         y_date + 2
            beq         @day
            jsr         y_monlen
            jsr         y_adddays
            inc         y_date + 2
            bra         @mon
@day:
            pla
            sta         y_date + 2
            lda         y_date + 3
            sec
            sbc         #1
            sta         y_n
            stz         y_n + 1
; y_days = y_days + y_n
y_adddays:
            clc
            lda         y_days
            adc         y_n
            sta         y_days
            lda         y_days + 1
            adc         y_n + 1
            sta         y_days + 1
            rts

; n_w32 = n_w32 * .A + .X (bytes)
y_muladd:
            sta         y_c
            stx         y_cnt
            ldx         #0
@byte:
            lda         n_w32,x                             ; (Each byte times .A, its carry the next's)
            phx
            ldx         y_c
            jsr         y_mul8
            plx
            clc
            adc         y_cnt
            sta         n_w32,x
            tya
            adc         #0
            sta         y_cnt
            inx
            cpx         #4
            bcc         @byte
            rts

; .A (low), .Y (high) = .A * .X
y_mul8:
            sta         y_mul
            stx         y_mul + 1
            lda         #0
            ldy         #8
@bit:
            asl
            rol         y_mulhi
            asl         y_mul
            bcc         :+
            clc
            adc         y_mul + 1
            bcc         :+
            inc         y_mulhi
:
            dey
            bne         @bit
            ldy         y_mulhi
            stz         y_mulhi
            rts

.bss
y_mul:      .res        2
y_mulhi:    .res        1
.segment "CODE5"

; ---- Where it's running

; (platform): :hydra
bi_platform:
            jsr         y_enter
            LDAX        s_hydra
            jsr         y_atom
            MOVW        ex, hv
            clc
            rts

; (hydra?): T
bi_hydra_p:
            jmp         y_t

.segment "RODATA5"
s_kname:    .byte       "name", 0
s_klength:  .byte       "length", 0
s_kdir:     .byte       "dir", 0
s_kmtime:   .byte       "mtime", 0
s_kmode:    .byte       "mode", 0
s_kqid:     .byte       "qid", 0
s_kdev:     .byte       "dev", 0
s_kyear:    .byte       "year", 0
part_lo:    .lobytes    s_kmonth, s_kday, s_khour, s_kminute, s_ksecond, s_kweekday
part_hi:    .hibytes    s_kmonth, s_kday, s_khour, s_kminute, s_ksecond, s_kweekday
s_kmonth:   .byte       "month", 0
s_kday:     .byte       "day", 0
s_khour:    .byte       "hour", 0
s_kminute:  .byte       "minute", 0
s_ksecond:  .byte       "second", 0
s_kweekday: .byte       "weekday", 0
mon_days:   .byte       31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
s_bin:      .byte       "/bin/"
s_rc:       .byte       "/bin/rc", 0
s_dashc:    .byte       "-c", 0
s_task:     .byte       "A task must be an integer", 0
s_sleep:    .byte       "'sleep' expects a number of seconds", 0
s_atime:    .byte       "a time must be an integer", 0
s_apart:    .byte       "A date's part must be an integer", 0
s_hydra:    .byte       "hydra", 0

.segment "CODE5"
