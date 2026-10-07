; ****************************************************************************
; mml.inc - play's scores (play song.mml): sim/tools/hysong.js's score language, compiled as the song plays into the
; stream hysong.js would write to a ZSM file (the same register writes at the same ticks; play -o writes it as one),
; which the player plays as it plays a file's.  Included by play.s.
;   A score (hysong.js's header has it whole): ';' a comment; #tempo N (quarter notes a minute; 120), #rate N (its
; ticks a second; 200), #title ...; @name { ... } an instrument (gm N, a patch of the driver's; or alg fb pms ams,
; then m1 m2 c1 c2 each with mul dt1 dt2 tl ks ar d1r d2r d1l rr am); A ... to H ..., MML for channels 0-7, a
; channel's lines joined in order.  MML: notes (c-b, + # -, a length, dots, ^ ties), r rests, x drums, o > < l q v
; p @ k D M L N y, _ slides, & legato, | bars, [ ... ]N repeats.
;   How: the score is read whole into memory (past the BSS: BREAK), its comments and instruments blanked out (the
; instruments kept in a table), its lines checked.  Each channel's MML is a track: a cursor through its lines (a
; repeat's start and end on a stack), its settings, and its writes waiting (a FIFO, each with its tick).  A note
; queues its attack's writes (its instrument's registers, its pitch, its key on) and leaves its key off and a
; slide's steps for later, so each track's writes come in the order of their ticks; the tracks' are merged by tick
; (a track before the ones after it: hysong.js sorts them so), the same register and value twice in a row left out
; (but key on and off, $01 and $19), each tick's pairs in chunks of 63, then the delay.  Times are in eighths of
; hysong.js's units (192 a whole note: a dot of a 64th is half of one), as 32-bit numbers; a tick is the time's,
; rounded (round (u * rate * 5 / (tempo * 32)), exactly), and a slide's steps' times and pitches the same way.
;   The compiled bytes go into a ring (m_ring) that the player's read ahead (stage_read: mml_stage) takes from, and
; that's filled as the player waits for its next tick (mml_idle), the time there is; at the start, all of it.

MML_INSTS       = 32            ; Instruments a score may name
INST_NAME       = 16            ; An instrument's name: 15 characters at most, then 0
PATCH_SIZE      = 26            ; A patch: $20, $38, then $40-$F8 by 8 (patches.s's)
INST_SIZE       = INST_NAME + PATCH_SIZE
FIFO            = 32            ; A track's writes waiting (a note's attack: 26 for its instrument, 2 its pitch, 2 key on)
RP_DEPTH        = 8             ; Repeats inside repeats
TRACKS          = 8
RING            = 1024          ; The compiled bytes ahead of the player
RING_ROOM       = 140           ; A step's bytes at most: a chunk of 63 pairs, and a delay

.zeropage
cp:         .res        2                                   ; The track's cursor: its next character ...
cl:         .res        2                                   ;   and its line (its first 2 bytes the next line's place)
mp:         .res        2                                   ; A pointer (an instrument, a patch, the text)
m_x:        .res        4                                   ; Scratch (32 bits), and a pointer
m_y:        .res        4                                   ; The same
m_save:     .res        2                                   ; A place in the text kept ...
m_savel:    .res        2                                   ;   and its line
trk:        .res        1                                   ; The track (its channel)
nv:         .res        2                                   ; num's number (signed)
dl:         .res        2                                   ; len's length (eighths of units)

.bss
mml:        .res        1                                   ; <> 0: the song is a score
m_buf:      .res        2                                   ; The score's text (a LF before it, a 0 after it) ...
m_end:      .res        2                                   ;   and its end
m_tempo:    .res        2
m_rate:     .res        2
m_rate10:   .res        2                                   ; rate * 10 ...
m_c:        .res        2                                   ;   tempo * 32 ...
m_d:        .res        2                                   ;   and tempo * 64 (a tick: round (u * rate10 / d))
m_mask:     .res        1                                   ; The channels with MML
m_ninst:    .res        1                                   ; The instruments ...
m_insts:    .res        MML_INSTS * INST_SIZE               ;   their names and patches
m_letter:   .res        1                                   ; The track's letter (getc's)
m_pk:       .res        1                                   ; (peek's)
m_cmd:      .res        1                                   ; The command's character
m_neg:      .res        1
m_add:      .res        2                                   ; (len's)
m_div:      .res        1                                   ; (len's)
m_at:       .res        3                                   ; A write's tick
m_reg:      .res        1                                   ; (A register ...
m_r:        .res        1                                   ;   and a count)
m_bit:      .res        1
m_slot:     .res        1                                   ; (put's)
m_carr:     .res        1                                   ; The carriers (a bit each: M1, M2, C1, C2)
m_att:      .res        1                                   ; The volume's attenuation
m_kc:       .res        1                                   ; kcf's key code ...
m_kf:       .res        1                                   ;   and fraction
m_pv:       .res        2                                   ; A pitch (64ths of a semitone, signed)
m_drum:     .res        1                                   ; 1: a drum note (its key code m_dkc)
m_dkc:      .res        1
m_glide:    .res        1                                   ; (play_note's)
m_er:       .res        1                                   ; (emit's: the register ...
m_ev:       .res        1                                   ;   and value)
m_now:      .res        3                                   ; The stream's tick
m_np:       .res        1                                   ; Pairs waiting for the stream ...
m_pairs:    .res        126                                 ;   and them
m_known:    .res        32                                  ; The registers written (a bit each) ...
m_shadow:   .res        256                                 ;   their values
m_longest:  .res        4                                   ; The longest track's time
m_ending:   .res        1                                   ; <> 0: every track done (the end's tick m_endt)
m_endt:     .res        3
m_done:     .res        1                                   ; The song's end in the stream
m_best:     .res        1                                   ; (m_next's: the track ...
m_bt:       .res        3                                   ;   and its head's tick)
m_msg:      .res        64                                  ; An error's text
m_a:        .res        4                                   ; tick's: the time (32 bits) ...
m_rem:      .res        2                                   ;   its fraction's numerator ...
m_q:        .res        2                                   ;   and denominator
m_w:        .res        2                                   ; mul's multiplier
m_len:      .res        1                                   ; (A word's length)
m_atpos:    .res        2                                   ; (instruments': the @)
m_ring:     .res        RING                                ; The compiled bytes: the oldest at m_rh ...
m_rh:       .res        2
m_rt:       .res        2                                   ;   the next's place ...
m_rn:       .res        2                                   ;   and how many there are
m_tick:     .res        2                                   ; (mml_idle's: the system tick to stop at)

; Each track's (TRACKS of each)
tk_cur_lo:  .res        TRACKS                              ; Its cursor ...
tk_cur_hi:  .res        TRACKS
tk_line_lo: .res        TRACKS                              ;   and its line
tk_line_hi: .res        TRACKS
tk_first_lo: .res       TRACKS                              ; Its first line (0: none)
tk_first_hi: .res       TRACKS
tk_end:     .res        TRACKS                              ; <> 0: its MML done
tk_u0:      .res        TRACKS                              ; Its time (32 bits)
tk_u1:      .res        TRACKS
tk_u2:      .res        TRACKS
tk_u3:      .res        TRACKS
tk_t0:      .res        TRACKS                              ; Its time's tick
tk_t1:      .res        TRACKS
tk_t2:      .res        TRACKS
tk_oct:     .res        TRACKS                              ; o
tk_len:     .res        TRACKS                              ; l
tk_gate:    .res        TRACKS                              ; q
tk_vol:     .res        TRACKS                              ; v
tk_pan:     .res        TRACKS                              ; p ($20's RL)
tk_inst_lo: .res        TRACKS                              ; Its instrument (a patch; 0: none) ...
tk_inst_hi: .res        TRACKS
tk_pend_lo: .res        TRACKS                              ;   the one for its next note ...
tk_pend_hi: .res        TRACKS
tk_name_lo: .res        TRACKS                              ;   and the one @ named last
tk_name_hi: .res        TRACKS
tk_lfo:     .res        TRACKS                              ; M's sensitivities ...
tk_lfoset:  .res        TRACKS                              ;   <> 0: set
tk_tr:      .res        TRACKS                              ; k
tk_det_lo:  .res        TRACKS                              ; D
tk_det_hi:  .res        TRACKS
tk_p_lo:    .res        TRACKS                              ; Its last note's pitch ...
tk_p_hi:    .res        TRACKS
tk_haspitch: .res       TRACKS                              ;   1: there is one
tk_legato:  .res        TRACKS                              ; &
tk_slide:   .res        TRACKS                              ; _
tk_rp:      .res        TRACKS                              ; Its repeats' depth
tk_fh:      .res        TRACKS                              ; Its FIFO: the head ...
tk_fn:      .res        TRACKS                              ;   and the writes in it
tk_koff:    .res        TRACKS                              ; <> 0: a key off to come ...
tk_k0:      .res        TRACKS                              ;   at this tick
tk_k1:      .res        TRACKS
tk_k2:      .res        TRACKS
tk_sl:      .res        TRACKS                              ; <> 0: a slide's steps to come: the next's tick ...
tk_sc0:     .res        TRACKS
tk_sc1:     .res        TRACKS
tk_sc2:     .res        TRACKS
tk_s00:     .res        TRACKS                              ;   its first's ...
tk_s01:     .res        TRACKS
tk_s02:     .res        TRACKS
tk_s10:     .res        TRACKS                              ;   its end's ...
tk_s11:     .res        TRACKS
tk_s12:     .res        TRACKS
tk_sq_lo:   .res        TRACKS                              ;   its ticks (at least 1) ...
tk_sq_hi:   .res        TRACKS
tk_sf_lo:   .res        TRACKS                              ;   the pitch from ...
tk_sf_hi:   .res        TRACKS
tk_st_lo:   .res        TRACKS                              ;   and to ...
tk_st_hi:   .res        TRACKS
tk_su0:     .res        TRACKS                              ;   the note's time ...
tk_su1:     .res        TRACKS
tk_su2:     .res        TRACKS
tk_su3:     .res        TRACKS
tk_sd_lo:   .res        TRACKS                              ;   and its length
tk_sd_hi:   .res        TRACKS
; Each track's FIFO (FIFO each: track * FIFO + its place) and repeats (RP_DEPTH each: track * RP_DEPTH + depth)
fi_t0:      .res        TRACKS * FIFO
fi_t1:      .res        TRACKS * FIFO
fi_t2:      .res        TRACKS * FIFO
fi_reg:     .res        TRACKS * FIFO
fi_val:     .res        TRACKS * FIFO
rp_s_lo:    .res        TRACKS * RP_DEPTH                   ; Its start (past the [) ...
rp_s_hi:    .res        TRACKS * RP_DEPTH
rp_e_lo:    .res        TRACKS * RP_DEPTH                   ;   its end (past the ] and its count) ...
rp_e_hi:    .res        TRACKS * RP_DEPTH
rp_sl_lo:   .res        TRACKS * RP_DEPTH                   ;   their lines ...
rp_sl_hi:   .res        TRACKS * RP_DEPTH
rp_el_lo:   .res        TRACKS * RP_DEPTH
rp_el_hi:   .res        TRACKS * RP_DEPTH
rp_n:       .res        TRACKS * RP_DEPTH                   ;   and the times it's still to be played
.code

; ****************************************************************************
; Loading the score

; Is the song's name a score's (it ends in .mml, in either case)?  OUT: mml
mml_name:
            stz         mml
            MOVR        mp, song
            ldy         #0
:
            lda         (mp),Y
            beq         :+
            iny
            bne         :-
:
            cpy         #4
            bcc         @no
            ldx         #3
@char:
            dey
            lda         (mp),Y
            ora         #$20
            cmp         s_mml,X
            bne         @no
            dex
            bpl         @char
            inc         mml
@no:
            rts

; The score (fd) read whole into memory past the BSS, a LF before it and a 0 after it; its comments blanked, its
; instruments read and blanked, its lines checked: #tempo, #rate, the channels with MML (m_mask).  A score that's
; wrong: fail's message (and the end)
mml_load:
            stz         r0                                  ; The break, and the text from it
            stz         r0 + 1
            jsr         BREAK
            MOVR        m_buf, r0
            MOVR        m_end, r0
            jsr         @room
            MOVR        mp, m_end
            lda         #LF
            sta         (mp)
            inc         m_end
            bne         @read
            inc         m_end + 1
@read:
            MOVR        r0, m_end
            LDR         r1, 256
            lda         fd
            jsr         READ
            bcc         :+
            jmp         fail_os
:
            stx         t + 1
            sta         t
            ora         t + 1
            beq         @all
            clc
            lda         m_end
            adc         t
            sta         m_end
            lda         m_end + 1
            adc         t + 1
            sta         m_end + 1
            jsr         @room
            bra         @read
@all:
            MOVR        mp, m_end
            lda         #0
            sta         (mp)
            lda         #120
            sta         m_tempo
            stz         m_tempo + 1
            lda         #200
            sta         m_rate
            stz         m_rate + 1
            stz         m_ninst
            stz         m_mask
            jsr         uncomment
            jsr         instruments
            jsr         lines
            jmp         scale

; (The break: room for 256 more bytes and a 0 past m_end, below $8000)
@room:
            clc
            lda         m_end
            adc         #<258
            sta         r0
            lda         m_end + 1
            adc         #>258
            sta         r0 + 1
            bcs         @big
            cmp         #>$8000
            bcs         @big
            jsr         BREAK
            bcs         @big
            rts
@big:
            LDR         r0, s_big
            jmp         fail

; ; to the line's end: blanks
uncomment:
            MOVR        mp, m_buf
@char:
            lda         (mp)
            beq         @done
            cmp         #';'
            bne         @next
@blank:
            lda         (mp)
            beq         @done
            cmp         #LF
            beq         @next
            lda         #' '
            sta         (mp)
            jsr         mp_inc
            bra         @blank
@next:
            jsr         mp_inc
            bra         @char
@done:
            rts

mp_inc:
            inc         mp
            bne         :+
            inc         mp + 1
:
            rts

cp_inc:
            inc         cp
            bne         :+
            inc         cp + 1
:
            rts

; Is .A a name's character (a letter, a digit, _)?  OUT: C = 1 yes.  Keeps .A, .X, .Y
is_word:
            cmp         #'_'
            beq         @yes
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         @yes
            pha
            ora         #$20
            cmp         #'a'
            bcc         @not
            cmp         #'z' + 1
            bcs         @not
            pla
@yes:
            sec
            rts
@not:
            pla
@no:
            clc
            rts

; Is .A a blank (space, tab, CR, LF)?  OUT: Z = 1 yes.  Keeps .A, .X, .Y
is_blank:
            cmp         #' '
            beq         :+
            cmp         #TAB
            beq         :+
            cmp         #CR
            beq         :+
            cmp         #LF
:
            rts

; The instruments, @name { ... } (anywhere: hysong.js takes them out of the text before its lines), each read into
; m_insts and blanked
instruments:
            MOVR        cp, m_buf
@find:
            lda         (cp)
            bne         :+
            rts
:
            cmp         #'@'
            beq         :+
            jsr         cp_inc
            bra         @find
:
            MOVR        m_atpos, cp                         ; (The @: blanked from here if it's a definition)
            jsr         cp_inc
            lda         (cp)                                ; Its name
            jsr         is_word
            bcc         @find
            MOVR        m_save, cp
:
            jsr         cp_inc
            lda         (cp)
            jsr         is_word
            bcs         :-
:
            lda         (cp)                                ; Blanks, then {: a definition
            jsr         is_blank
            bne         :+
            jsr         cp_inc
            bra         :-
:
            cmp         #'{'
            bne         @find
            jsr         define
            bra         @find

; An instrument defined: its name at m_save (to the first character that isn't a name's), its body from cp (the
; {) to the }; then all of it, from the @ (m_atpos), blanked
define:
            lda         m_ninst
            cmp         #MML_INSTS
            bcc         :+
            LDR         r0, s_toomany
            jmp         fail
:
            jsr         inst_ptr                            ; m_x: its place
            ldy         #0
@name:
            lda         (m_save),Y
            jsr         is_word
            bcc         :+
            cpy         #INST_NAME - 1
            bcs         @bad
            sta         (m_x),Y
            iny
            bra         @name
:
            lda         #0
            sta         (m_x),Y
            ldy         #INST_NAME                          ; The patch: zeros, the TLs 127 (silent: hysong.js's)
:
            lda         #0
            sta         (m_x),Y
            iny
            cpy         #INST_SIZE
            bcc         :-
            ldy         #INST_NAME + 6
:
            lda         #127
            sta         (m_x),Y
            iny
            cpy         #INST_NAME + 10
            bcc         :-
            jsr         cp_inc                              ; Its body: words
            lda         #$FF                                ; (The operator: none yet)
            sta         m_r
@word:
            jsr         body_word
            bcs         @end
            jsr         voice_word
            bra         @word
@end:
            inc         m_ninst
            MOVR        mp, m_atpos
@blank:                                                     ; From the @ to the }: blanks
            lda         #' '
            sta         (mp)
            lda         mp
            cmp         cp
            bne         :+
            lda         mp + 1
            cmp         cp + 1
            beq         @blanked
:
            jsr         mp_inc
            bra         @blank
@blanked:
            jmp         cp_inc

@bad:
            LDR         r0, s_badinst
            jmp         fail

; m_x = instrument .A's place in m_insts
inst_ptr:
            sta         m_x
            stz         m_x + 1
            lda         #INST_SIZE
            sta         m_w
            stz         m_w + 1
            jsr         mul16
            clc
            lda         m_x
            adc         #<m_insts
            sta         m_x
            lda         m_x + 1
            adc         #>m_insts
            sta         m_x + 1
            rts

; The body's next word: m_y = its start, .Y its length (cp past it).  OUT: C = 1 at the } (cp on it); a body with no
; } (the text's end): the instrument's error
body_word:
@skip:
            lda         (cp)
            bne         :+
            LDR         r0, s_badinst
            jmp         fail
:
            cmp         #'}'
            bne         :+
            sec
            rts
:
            jsr         is_blank
            bne         @word
            jsr         cp_inc
            bra         @skip
@word:
            MOVR        m_y, cp
            ldy         #0
:
            lda         (cp),Y
            beq         :+
            cmp         #'}'
            beq         :+
            jsr         is_blank
            beq         :+
            iny
            bne         :-
:
            tya                                             ; (cp past it)
            clc
            adc         cp
            sta         cp
            bcc         :+
            inc         cp + 1
:
            clc
            rts

; A word of an instrument's body (at m_y, .Y long), with its number after it if it takes one: gm N (the driver's
; patch N, all of it), alg, fb, pms, ams, an operator (m1 m2 c1 c2), or one of its settings (mul dt1 dt2 tl ks ar
; d1r d2r d1l rr am)
voice_word:
            sty         m_len
            ldx         #0
@try:
            lda         voice_lo,X                          ; Each name in turn
            sta         mp
            lda         voice_hi,X
            beq         @bad                                ; (None: not one)
            sta         mp + 1
            ldy         #0
:
            lda         (mp),Y
            beq         @end
            cmp         (m_y),Y
            bne         @next
            iny
            bra         :-
@end:
            cpy         m_len
            beq         @found
@next:
            inx
            bra         @try
@bad:
            LDR         r0, s_badinst
            jmp         fail

@found:
            cpx         #V_M1                               ; An operator: the settings after it its
            bcc         :+
            cpx         #V_C2 + 1
            bcs         :+
            txa
            sec
            sbc         #V_M1
            sta         m_r
            rts
:
            phx                                             ; Its number
            jsr         body_word
            bcs         @nonum
            jsr         word_num
@nonum:
            plx
            bcs         @bad
            cpx         #V_GM
            beq         @gm
            lda         voice_reg,X                         ; Its byte: the voice's, or its operator's
            bpl         @set
            ldy         m_r                                 ; (An operator's setting: none chosen, an error)
            bmi         @bad
            and         #$7F
            clc
            adc         m_r
@set:
            tay
            lda         voice_mask,X                        ; The field: (byte & ~mask) | (n << shift) & mask
            eor         #$FF
            and         (m_x),Y
            sta         m_reg
            lda         nv
            phx
            pha
            lda         voice_shift,X
            tax
            pla
            cpx         #0
            beq         :++
:
            asl
            dex
            bne         :-
:
            plx
            and         voice_mask,X
            ora         m_reg
            sta         (m_x),Y
            rts

@gm:
            lda         nv + 1                              ; gm N: the driver's patch N
            bne         @bad
            lda         nv
            cmp         #163
            bcs         @bad
            jsr         patch_of
            ldy         #0
:
            lda         (mp),Y
            phy
            pha
            tya
            clc
            adc         #INST_NAME
            tay
            pla
            sta         (m_x),Y
            ply
            iny
            cpy         #PATCH_SIZE
            bcc         :-
            rts

; mp = the driver's patch .A (patches + .A * 26).  Keeps .X
patch_of:
            sta         m_w
            stz         m_w + 1
            lda         #PATCH_SIZE
            sta         mp
            stz         mp + 1
            stz         m_rem
            stz         m_rem + 1
            phx
            ldx         #8
:
            lsr         m_w
            bcc         :+
            clc
            lda         m_rem
            adc         mp
            sta         m_rem
            lda         m_rem + 1
            adc         mp + 1
            sta         m_rem + 1
:
            asl         mp
            rol         mp + 1
            dex
            bne         :--
            plx
            clc
            lda         m_rem
            adc         #<patches
            sta         mp
            lda         m_rem + 1
            adc         #>patches
            sta         mp + 1
            rts

; nv = the number at m_y (.Y long: decimal, a - before it allowed).  OUT: C = 1, not one.  Keeps .X
word_num:
            stz         nv
            stz         nv + 1
            stz         m_neg
            sty         m_len
            cpy         #0
            beq         @no
            ldy         #0
            lda         (m_y)
            cmp         #'-'
            bne         @digit
            inc         m_neg
            iny
            cpy         m_len
            beq         @no
@digit:
            cpy         m_len
            beq         @end
            lda         (m_y),Y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @no
            jsr         nv_digit
            iny
            bra         @digit
@end:
            lda         m_neg
            beq         :+
            jsr         nv_neg
:
            clc
            rts
@no:
            sec
            rts

; nv = nv * 10 + .A.  Keeps .X, .Y
nv_digit:
            pha
            lda         nv
            sta         m_w
            lda         nv + 1
            sta         m_w + 1
            asl         nv
            rol         nv + 1
            asl         nv
            rol         nv + 1
            clc
            lda         nv
            adc         m_w
            sta         nv
            lda         nv + 1
            adc         m_w + 1
            sta         nv + 1
            asl         nv
            rol         nv + 1
            pla
            clc
            adc         nv
            sta         nv
            bcc         :+
            inc         nv + 1
:
            rts

nv_neg:
            sec
            lda         #0
            sbc         nv
            sta         nv
            lda         #0
            sbc         nv + 1
            sta         nv + 1
            rts

; The lines: each, its blanks trimmed, empty; #tempo N, #rate N or #title ...; or A-H, blanks, its MML (the channel's
; bit in m_mask).  Anything else: "what is this line?"  A channel's lines are linked, its first in tk_first: each
; one's letter and the blank after it become the place of the channel's next line (0 after its last), so its
; cursor goes from one to the next at once
lines:
            ldx         #TRACKS - 1
:
            stz         tk_first_lo,X
            stz         tk_first_hi,X
            stz         tk_line_lo,X                        ; (Each one's last so far: none)
            stz         tk_line_hi,X
            dex
            bpl         :-
            MOVR        cp, m_buf
            jsr         @lines
            ldx         #TRACKS - 1                         ; Each one's last line: 0, the end
@ends:
            lda         tk_line_hi,X
            beq         :+
            sta         mp + 1
            lda         tk_line_lo,X
            sta         mp
            lda         #0
            sta         (mp)
            ldy         #1
            sta         (mp),Y
:
            dex
            bpl         @ends
            rts

@lines:
@line:
            jsr         cp_inc                              ; (Past the LF)
@blank:
            lda         (cp)
            bne         :+
            rts
:
            cmp         #LF
            beq         @line
            jsr         is_blank
            bne         :+
            jsr         cp_inc
            bra         @blank
:
            cmp         #'#'
            beq         @hash
            sec
            sbc         #'A'
            cmp         #TRACKS
            bcs         @what
            tax
            ldy         #1
            lda         (cp),Y
            cmp         #' '
            beq         :+
            cmp         #TAB
            bne         @what
:
            lda         bits,X
            ora         m_mask
            sta         m_mask
            lda         tk_line_hi,X                        ; Linked: the last one's place, or the first
            beq         @first
            sta         mp + 1
            lda         tk_line_lo,X
            sta         mp
            lda         cp
            sta         (mp)
            ldy         #1
            lda         cp + 1
            sta         (mp),Y
            bra         @last
@first:
            lda         cp
            sta         tk_first_lo,X
            lda         cp + 1
            sta         tk_first_hi,X
@last:
            lda         cp
            sta         tk_line_lo,X
            lda         cp + 1
            sta         tk_line_hi,X
@skip:                                                      ; To the line's end
            lda         (cp)
            beq         @done
            cmp         #LF
            beq         @line
            jsr         cp_inc
            bra         @skip
@done:
            rts

@what:
            LDR         r0, s_what
            jmp         fail

@hash:
            jsr         cp_inc
            MOVR        m_y, cp
            ldy         #0
:
            lda         (cp),Y
            jsr         is_word
            bcc         :+
            iny
            bra         :-
:
            sty         m_len
            ldx         #0                                  ; Which: tempo, rate or title
@which:
            lda         hash_lo,X
            sta         mp
            lda         hash_hi,X
            beq         @hashbad
            sta         mp + 1
            ldy         #0
:
            lda         (mp),Y
            beq         :+
            cmp         (m_y),Y
            bne         @nextword
            iny
            bra         :-
:
            cpy         m_len
            beq         @found
@nextword:
            inx
            bra         @which
@hashbad:
            LDR         r0, s_hash
            jmp         fail
@found:
            cpx         #2                                  ; (#title: the rest ignored)
            beq         @skip
            phx
            tya                                             ; Its number, after blanks
            clc
            adc         cp
            sta         cp
            bcc         :+
            inc         cp + 1
:
            lda         (cp)
            cmp         #' '
            beq         :+
            cmp         #TAB
            bne         :++
:
            jsr         cp_inc
            bra         :--
:
            MOVR        m_y, cp
            ldy         #0
:
            lda         (cp),Y
            cmp         #'0'
            bcc         :+
            cmp         #'9' + 1
            bcs         :+
            iny
            bra         :-
:
            jsr         word_num
            plx
            bcs         @hashbad
            lda         nv                                  ; (0: hysong.js's would divide by it)
            ora         nv + 1
            beq         @hashbad
            lda         nv
            ldy         nv + 1
            cpx         #0
            bne         :+
            sta         m_tempo
            sty         m_tempo + 1
            jmp         @skip
:
            sta         m_rate
            sty         m_rate + 1
            jmp         @skip

; rate * 10, tempo * 32 and * 64 (tick's), and the header's rate
scale:
            lda         m_rate
            sta         hdr + H_RATE
            sta         m_x
            lda         m_rate + 1
            sta         hdr + H_RATE + 1
            sta         m_x + 1
            lda         #10
            sta         m_w
            stz         m_w + 1
            jsr         mul16
            MOVR        m_rate10, m_x
            lda         m_tempo
            sta         m_c
            lda         m_tempo + 1
            sta         m_c + 1
            ldx         #5
:
            asl         m_c
            rol         m_c + 1
            dex
            bne         :-
            lda         m_c
            asl
            sta         m_d
            lda         m_c + 1
            rol
            sta         m_d + 1
            rts

; m_x = m_x * m_w (16 bits).  Keeps .X, .Y
mul16:
            phx
            lda         m_x
            sta         m_y
            lda         m_x + 1
            sta         m_y + 1
            stz         m_x
            stz         m_x + 1
            ldx         #16
:
            asl         m_x
            rol         m_x + 1
            asl         m_w
            rol         m_w + 1
            bcc         :+
            clc
            lda         m_x
            adc         m_y
            sta         m_x
            lda         m_x + 1
            adc         m_y + 1
            sta         m_x + 1
:
            dex
            bne         :--
            plx
            rts

; tl_num = tl_num * m_w (32 bits by 16; the low 32 kept; m_y used).  Keeps .Y
mul32:
            phx
            ldx         #3
:
            lda         tl_num,X
            sta         m_y,X
            stz         tl_num,X
            dex
            bpl         :-
            ldx         #16
@bit:
            asl         tl_num
            rol         tl_num + 1
            rol         tl_num + 2
            rol         tl_num + 3
            asl         m_w
            rol         m_w + 1
            bcc         @next
            clc
            lda         tl_num
            adc         m_y
            sta         tl_num
            lda         tl_num + 1
            adc         m_y + 1
            sta         tl_num + 1
            lda         tl_num + 2
            adc         m_y + 2
            sta         tl_num + 2
            lda         tl_num + 3
            adc         m_y + 3
            sta         tl_num + 3
@next:
            dex
            bne         @bit
            plx
            rts

; ****************************************************************************
; Ticks

; tl_num = m_a * rate10, divided by d: tl_num the quotient, tl_rem the remainder
tick_q:
            ldx         #3
:
            lda         m_a,X
            sta         tl_num,X
            dex
            bpl         :-
            MOVR        m_w, m_rate10
            jsr         mul32
            lda         m_d
            sta         tl_den
            lda         m_d + 1
            sta         tl_den + 1
            stz         tl_den + 2
            stz         tl_den + 3
            jmp         tl_div

; m_at = the tick of time m_a (whole eighths): round (m_a * rate10 / d) = q + (r + c >= d), as c is d / 2
tick_at:
            jsr         tick_q
            lda         tl_num
            sta         m_at
            lda         tl_num + 1
            sta         m_at + 1
            lda         tl_num + 2
            sta         m_at + 2
            clc                                             ; r + c >= d?
            lda         tl_rem
            adc         m_c
            tax
            lda         tl_rem + 1
            adc         m_c + 1
            tay
            txa
            cmp         m_d
            tya
            sbc         m_d + 1
            bcc         @done
            inc         m_at
            bne         @done
            inc         m_at + 1
            bne         @done
            inc         m_at + 2
@done:
            rts

; m_at = the tick of time m_a + m_rem / m_q (m_q at least 1), exactly: q, r = (m_a * rate10) divmod d, then
; q + (r * m_q + m_rem * rate10 + c * m_q) / (d * m_q)
tick_of:
            jsr         tick_q
            lda         tl_num                              ; m_at = q
            sta         m_at
            lda         tl_num + 1
            sta         m_at + 1
            lda         tl_num + 2
            sta         m_at + 2
            lda         tl_rem                              ; m_x = r * m_q ...
            sta         tl_num
            lda         tl_rem + 1
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            MOVR        m_w, m_q
            jsr         mul32
            ldx         #3
:
            lda         tl_num,X
            sta         m_x,X
            dex
            bpl         :-
            lda         m_rem                               ;   + m_rem * rate10 ...
            sta         tl_num
            lda         m_rem + 1
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            MOVR        m_w, m_rate10
            jsr         mul32
            jsr         x_add
            lda         m_c                                 ;   + c * m_q
            sta         tl_num
            lda         m_c + 1
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            MOVR        m_w, m_q
            jsr         mul32
            jsr         x_add
            lda         m_d                                 ; / (d * m_q)
            sta         tl_num
            lda         m_d + 1
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            MOVR        m_w, m_q
            jsr         mul32
            ldx         #3
:
            lda         tl_num,X
            sta         tl_den,X
            lda         m_x,X
            sta         tl_num,X
            dex
            bpl         :-
            jsr         tl_div
            clc
            lda         m_at
            adc         tl_num
            sta         m_at
            lda         m_at + 1
            adc         tl_num + 1
            sta         m_at + 1
            lda         m_at + 2
            adc         tl_num + 2
            sta         m_at + 2
            rts

; m_x += tl_num (32 bits)
x_add:
            clc
            ldx         #0
:
            lda         m_x,X
            adc         tl_num,X
            sta         m_x,X
            inx
            txa                                             ; (C kept: cpx would change it)
            eor         #4
            bne         :-
            rts

; m_a = track trk's time
u_to_a:
            ldx         trk
            lda         tk_u0,X
            sta         m_a
            lda         tk_u1,X
            sta         m_a + 1
            lda         tk_u2,X
            sta         m_a + 2
            lda         tk_u3,X
            sta         m_a + 3
            rts

; m_a += tl_num's low 2 bytes
a_add:
            clc
            lda         m_a
            adc         tl_num
            sta         m_a
            lda         m_a + 1
            adc         tl_num + 1
            sta         m_a + 1
            bcc         :+
            inc         m_a + 2
            bne         :+
            inc         m_a + 3
:
            rts

; Track trk's time += dl, and its tick worked out again
u_add:
            ldx         trk
            clc
            lda         tk_u0,X
            adc         dl
            sta         tk_u0,X
            lda         tk_u1,X
            adc         dl + 1
            sta         tk_u1,X
            lda         tk_u2,X
            adc         #0
            sta         tk_u2,X
            lda         tk_u3,X
            adc         #0
            sta         tk_u3,X
            jsr         u_to_a
            jsr         tick_at
            ldx         trk
            lda         m_at
            sta         tk_t0,X
            lda         m_at + 1
            sta         tk_t1,X
            lda         m_at + 2
            sta         tk_t2,X
            rts

; m_at = track trk's time's tick
t_to_at:
            ldx         trk
            lda         tk_t0,X
            sta         m_at
            lda         tk_t1,X
            sta         m_at + 1
            lda         tk_t2,X
            sta         m_at + 2
            rts

; ****************************************************************************
; A track's text

; Every track at its start: its cursor at its first line's MML, its settings hysong.js's; the stream at its start
mml_reset:
            ldx         #TRACKS - 1
@track:
            lda         tk_first_lo,X
            sta         tk_line_lo,X
            clc
            adc         #2
            sta         tk_cur_lo,X
            lda         tk_first_hi,X
            sta         tk_line_hi,X
            adc         #0
            sta         tk_cur_hi,X
            lda         m_mask                              ; (A channel with no MML: done)
            and         bits,X
            eor         bits,X
            sta         tk_end,X
            lda         #4
            sta         tk_oct,X
            sta         tk_len,X
            lda         #7
            sta         tk_gate,X
            lda         #100
            sta         tk_vol,X
            lda         #$C0
            sta         tk_pan,X
            stz         tk_u0,X
            stz         tk_u1,X
            stz         tk_u2,X
            stz         tk_u3,X
            stz         tk_t0,X
            stz         tk_t1,X
            stz         tk_t2,X
            stz         tk_inst_lo,X
            stz         tk_inst_hi,X
            stz         tk_pend_lo,X
            stz         tk_pend_hi,X
            stz         tk_name_lo,X
            stz         tk_name_hi,X
            stz         tk_lfoset,X
            stz         tk_tr,X
            stz         tk_det_lo,X
            stz         tk_det_hi,X
            stz         tk_haspitch,X
            stz         tk_legato,X
            stz         tk_slide,X
            stz         tk_rp,X
            stz         tk_fh,X
            stz         tk_fn,X
            stz         tk_koff,X
            stz         tk_sl,X
            dex
            bmi         :+
            jmp         @track
:
            stz         m_now
            stz         m_now + 1
            stz         m_now + 2
            stz         m_np
            stz         m_done
            stz         m_ending
            ldx         #3
:
            stz         m_longest,X
            dex
            bpl         :-
            ldx         #31
:
            stz         m_known,X
            dex
            bpl         :-
            rts

; cp and cl: track trk's cursor and line
cur_get:
            ldx         trk
            lda         tk_cur_lo,X
            sta         cp
            lda         tk_cur_hi,X
            sta         cp + 1
            lda         tk_line_lo,X
            sta         cl
            lda         tk_line_hi,X
            sta         cl + 1
            rts

cur_put:
            ldx         trk
            lda         cp
            sta         tk_cur_lo,X
            lda         cp + 1
            sta         tk_cur_hi,X
            lda         cl
            sta         tk_line_lo,X
            lda         cl + 1
            sta         tk_line_hi,X
            rts

; The track's next character (cp on past it), its lines joined by a blank: at a line's end (a LF, or the text's), on
; to the track's next line (its place at this one's start).  OUT: .A, Z = 1 at its end (0: cp on the text's 0).
; Keeps .X, .Y
getc:
            lda         (cp)
            beq         @line
            cmp         #LF
            beq         @line
            jsr         cp_inc
            cmp         #0                                  ; (Z = 0)
            rts

@line:
            phy
            lda         (cl)
            pha
            ldy         #1
            lda         (cl),Y
            bne         @next
            pla                                             ; (None: the end)
            ply
            lda         m_end
            sta         cp
            lda         m_end + 1
            sta         cp + 1
            lda         #0
            rts
@next:
            sta         cl + 1
            sta         cp + 1
            pla
            sta         cl
            clc                                             ; (Its MML: past the 2 bytes)
            adc         #2
            sta         cp
            bcc         :+
            inc         cp + 1
:
            ply
            lda         #' '                                ; (Z = 0)
            rts

; The next character, cp left where it was.  OUT: .A (0 at the end), Z.  Keeps .X, .Y
peek:
            lda         cp
            pha
            lda         cp + 1
            pha
            lda         cl
            pha
            lda         cl + 1
            pha
            jsr         getc
            sta         m_pk
            pla
            sta         cl + 1
            pla
            sta         cl
            pla
            sta         cp + 1
            pla
            sta         cp
            lda         m_pk
            rts

; Past the blanks.  Keeps .X, .Y
ws:
            jsr         peek
            beq         @done
            cmp         #' ' + 1
            bcs         @done
            jsr         getc
            bra         ws
@done:
            rts

; A number after the blanks: decimal or $hex, a - before it.  OUT: C = 0, nv (signed); or C = 1: none (cp past
; the blanks only)
num:
            jsr         ws
            MOVR        m_save, cp
            MOVR        m_savel, cl
            stz         m_neg
            stz         nv
            stz         nv + 1
            jsr         peek
            cmp         #'-'
            bne         :+
            jsr         getc
            inc         m_neg
:
            ldx         #0
            jsr         peek
            cmp         #'$'
            beq         @hex
@dec:
            jsr         peek
            sec
            sbc         #'0'
            cmp         #10
            bcs         @end
            jsr         nv_digit
            jsr         getc
            inx
            bra         @dec
@hex:
            jsr         getc
@hexdigit:
            jsr         peek
            jsr         hexval
            bcs         @end
            asl         nv
            rol         nv + 1
            asl         nv
            rol         nv + 1
            asl         nv
            rol         nv + 1
            asl         nv
            rol         nv + 1
            ora         nv
            sta         nv
            jsr         getc
            inx
            bra         @hexdigit
@end:
            cpx         #0
            beq         @none
            lda         m_neg
            beq         :+
            jsr         nv_neg
:
            clc
            rts
@none:
            MOVR        cp, m_save
            MOVR        cl, m_savel
            sec
            rts

; .A's value as a hex digit.  OUT: C = 0, .A; or C = 1, not one
hexval:
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcs         :+
            sbc         #'0' - 1                            ; (C = 0: one less)
            clc
            rts
:
            ora         #$20
            cmp         #'a'
            bcc         @no
            cmp         #'f' + 1
            bcs         @no
            sbc         #'a' - 11                           ; (C = 0: one less)
            clc
            rts
@no:
            sec
            rts

; A number there must be (the track's error if not)
num_must:
            jsr         num
            bcc         :+
            LDR         r0, s_nonum
            jmp         track_fail
:
            rts

; A length: a number (the track's l if none: 1-192, dividing 192, a whole note's units), dots (each half the last
; added: eighths of a unit at least), ^ and another length tied on.  OUT: dl (eighths of units)
len:
            jsr         num
            bcc         :+
            ldx         trk
            lda         tk_len,X
            sta         nv
            stz         nv + 1
:
            lda         nv + 1
            bne         @bad
            lda         nv
            beq         @bad
            cmp         #193
            bcs         @bad
            sta         m_div                               ; 192 / n: none left over
            lda         #192
            ldx         #0
:
            cmp         m_div
            bcc         :+
            sbc         m_div
            inx
            bra         :-
:
            cmp         #0
            bne         @bad
            stx         dl                                  ; * 8
            stz         dl + 1
            asl         dl
            rol         dl + 1
            asl         dl
            rol         dl + 1
            asl         dl
            rol         dl + 1
            MOVR        m_add, dl
@dot:
            jsr         peek
            cmp         #'.'
            bne         @tie
            jsr         getc
            lsr         m_add + 1
            ror         m_add
            bcs         @bad                                ; (Finer than an eighth of a unit)
            clc
            lda         dl
            adc         m_add
            sta         dl
            lda         dl + 1
            adc         m_add + 1
            sta         dl + 1
            bra         @dot
@tie:
            jsr         ws
            jsr         peek
            cmp         #'^'
            bne         @done
            jsr         getc
            lda         dl
            pha
            lda         dl + 1
            pha
            jsr         len
            pla
            tax
            pla
            clc
            adc         dl
            sta         dl
            txa
            adc         dl + 1
            sta         dl + 1
@done:
            rts

@bad:
            LDR         r0, s_badlen
            jmp         track_fail

; ****************************************************************************
; A track's writes

; Register .A = .Y at tick m_at into track trk's FIFO
put:
            pha
            ldx         trk
            lda         tk_fn,X
            cmp         #FIFO
            bcc         :+
            LDR         r0, s_toomuch
            jmp         track_fail
:
            clc
            adc         tk_fh,X
            and         #FIFO - 1
            sta         m_slot
            txa
            asl
            asl
            asl
            asl
            asl
            ora         m_slot
            tax
            pla
            sta         fi_reg,X
            tya
            sta         fi_val,X
            lda         m_at
            sta         fi_t0,X
            lda         m_at + 1
            sta         fi_t1,X
            lda         m_at + 2
            sta         fi_t2,X
            ldx         trk
            inc         tk_fn,X
            rts

; The same at the track's time
put_now:
            pha
            phy
            jsr         t_to_at
            ply
            pla
            jmp         put

; m_att = the attenuation of track trk's volume (0-127: hysong.js's atten)
att_of:
            ldx         trk
            lda         tk_vol,X
            tax
            lda         volume_atten,X
            sta         m_att
            rts

; The pending instrument into the channel at its time (hysong.js's patch): $20 (the pan, and the patch's feedback and
; algorithm), $38 (M's sensitivities, or the patch's), then $40-$F8, the carriers' levels with the volume's
; attenuation
load_inst:
            ldx         trk
            lda         tk_pend_lo,X
            sta         mp
            sta         tk_inst_lo,X
            lda         tk_pend_hi,X
            sta         mp + 1
            sta         tk_inst_hi,X
            stz         tk_pend_lo,X
            stz         tk_pend_hi,X
            lda         (mp)
            and         #$3F
            ora         tk_pan,X
            tay
            lda         trk
            ora         #$20
            jsr         put_now
            ldx         trk
            lda         tk_lfoset,X
            beq         :+
            lda         tk_lfo,X
            bra         :++
:
            ldy         #1
            lda         (mp),Y
:
            tay
            lda         trk
            ora         #$38
            jsr         put_now
            jsr         att_of
            lda         (mp)
            and         #7
            tax
            lda         carriers,X
            sta         m_carr
            stz         m_r                                 ; r: 0-23
@reg:
            lda         m_r                                 ; $40 + (r >> 2) * $20 + (r & 3) * 8 + ch
            lsr
            lsr
            tax
            lda         reg_rows,X
            sta         m_reg
            lda         m_r
            and         #3
            asl
            asl
            asl
            ora         m_reg
            ora         trk
            sta         m_reg
            lda         m_r
            clc
            adc         #2
            tay
            lda         (mp),Y
            cpx         #1                                  ; (TL, a carrier's: the attenuation added)
            bne         @put
            pha
            lda         m_r
            and         #3
            tax
            lda         bits,X
            and         m_carr
            beq         @plain
            pla
            clc
            adc         m_att
            bpl         @put
            lda         #127
            bra         @put
@plain:
            pla
@put:
            tay
            lda         m_reg
            jsr         put_now
            inc         m_r
            lda         m_r
            cmp         #24
            bcc         @reg
            rts

; The carriers' levels again (a new volume), if the track has an instrument
levels:
            ldx         trk
            lda         tk_inst_hi,X
            bne         :+
            rts
:
            sta         mp + 1
            lda         tk_inst_lo,X
            sta         mp
            jsr         att_of
            lda         (mp)
            and         #7
            tax
            lda         carriers,X
            sta         m_carr
            ldx         #0
@slot:
            lda         bits,X
            and         m_carr
            beq         @next
            phx
            txa                                             ; $60 + slot * 8 + ch = TL[slot] + atten
            asl
            asl
            asl
            ora         #$60
            ora         trk
            sta         m_reg
            txa
            clc
            adc         #6
            tay
            lda         (mp),Y
            clc
            adc         m_att
            bpl         :+
            lda         #127
:
            tay
            lda         m_reg
            jsr         put_now
            plx
@next:
            inx
            cpx         #4
            bne         @slot
            rts

; m_kc, m_kf: pitch m_pv (64ths of a semitone, MIDI's) as a key code and fraction (hysong.js's kcf: below 13, 13;
; past 108, 108 and 63 64ths)
kcf:
            lda         m_pv                                ; n = pitch >> 6 (signed), kf = pitch & 63
            and         #$3F
            sta         m_kf
            lda         m_pv
            sta         m_x
            lda         m_pv + 1
            sta         m_x + 1
            ldx         #6
:
            lda         m_x + 1
            cmp         #$80
            ror         m_x + 1
            ror         m_x
            dex
            bne         :-
            lda         m_x + 1
            bmi         @low
            bne         @high
            lda         m_x
            cmp         #13
            bcc         @low
            cmp         #109
            bcs         @high
            bra         @code
@low:
            lda         #13
            stz         m_kf
            bra         @code
@high:
            lda         #63
            sta         m_kf
            lda         #108
@code:
            sec
            sbc         #13
            ldx         #0
:
            cmp         #12
            bcc         :+
            sbc         #12
            inx
            bra         :-
:
            tay
            txa
            asl
            asl
            asl
            asl
            ora         note_codes,Y
            sta         m_kc
            asl         m_kf
            asl         m_kf
            rts

; A note (hysong.js's play): pitch m_pv, length dl; a drum if m_drum (its key code m_dkc).  Its instrument first if
; one's pending; its pitch (or a slide's steps, for later); its key on, unless it's legato or a slide; & after it:
; legato; its key off, for later (at its length's q eighths, rounded: a unit at least), unless legato
play_note:
            ldx         trk                                 ; A glide: a slide, from a pitch, not a drum
            lda         tk_slide,X
            and         tk_haspitch,X
            beq         :+
            lda         m_drum
            eor         #1
:
            sta         m_glide
            lda         tk_pend_hi,X
            beq         :+
            jsr         load_inst
:
            lda         m_glide
            beq         @pitch
            jsr         slide_start
            bra         @attack
@pitch:
            lda         m_drum
            beq         :+
            lda         m_dkc
            sta         m_kc
            stz         m_kf
            bra         :++
:
            jsr         kcf
:
            ldy         m_kc
            lda         trk
            ora         #$28
            jsr         put_now
            ldy         m_kf
            lda         trk
            ora         #$30
            jsr         put_now
@attack:
            ldx         trk                                 ; Key on: not legato or a glide (a drum always)
            lda         m_drum
            bne         :+
            lda         tk_legato,X
            ora         m_glide
            bne         @pitched
:
            ldy         trk
            lda         #$08
            jsr         put_now
            lda         trk
            ora         #$78
            tay
            lda         #$08
            jsr         put_now
@pitched:
            ldx         trk                                 ; Its pitch (a drum: none)
            stz         tk_haspitch,X
            lda         m_drum
            bne         :+
            lda         m_pv
            sta         tk_p_lo,X
            lda         m_pv + 1
            sta         tk_p_hi,X
            inc         tk_haspitch,X
:
            stz         tk_legato,X                         ; & after it
            jsr         ws
            jsr         peek
            cmp         #'&'
            bne         :+
            jsr         getc
            ldx         trk
            inc         tk_legato,X
:
            ldx         trk
            stz         tk_slide,X
            lda         tk_legato,X
            bne         @on
            lda         dl                                  ; Its key off: at round (dl * q / 64) units (1 at least)
            sta         tl_num
            lda         dl + 1
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            lda         tk_gate,X
            sta         m_w
            stz         m_w + 1
            jsr         mul32
            clc
            lda         tl_num
            adc         #32
            sta         tl_num
            bcc         :+
            inc         tl_num + 1
            bne         :+
            inc         tl_num + 2
:
            ldx         #6                                  ; (/ 64)
:
            lsr         tl_num + 2
            ror         tl_num + 1
            ror         tl_num
            dex
            bne         :-
            lda         tl_num
            ora         tl_num + 1
            bne         :+
            inc         tl_num
:
            ldx         #3                                  ; (Eighths: * 8)
:
            asl         tl_num
            rol         tl_num + 1
            dex
            bne         :-
            jsr         u_to_a
            jsr         a_add
            jsr         tick_at
            ldx         trk
            lda         m_at
            sta         tk_k0,X
            lda         m_at + 1
            sta         tk_k1,X
            lda         m_at + 2
            sta         tk_k2,X
            lda         #1
            sta         tk_koff,X
@on:
            jmp         u_add                               ; Its time on

; A slide from the last pitch to m_pv over the note's length (dl), its steps for later: every 2 ticks from the
; note's tick to its end's (hysong.js's: each step's time and pitch rounded as it rounds them)
slide_start:
            ldx         trk
            lda         tk_p_lo,X
            sta         tk_sf_lo,X
            lda         tk_p_hi,X
            sta         tk_sf_hi,X
            lda         m_pv
            sta         tk_st_lo,X
            lda         m_pv + 1
            sta         tk_st_hi,X
            lda         dl
            sta         tk_sd_lo,X
            lda         dl + 1
            sta         tk_sd_hi,X
            lda         tk_u0,X
            sta         tk_su0,X
            lda         tk_u1,X
            sta         tk_su1,X
            lda         tk_u2,X
            sta         tk_su2,X
            lda         tk_u3,X
            sta         tk_su3,X
            lda         tk_t0,X                             ; t0 (and the first step's) ...
            sta         tk_s00,X
            sta         tk_sc0,X
            lda         tk_t1,X
            sta         tk_s01,X
            sta         tk_sc1,X
            lda         tk_t2,X
            sta         tk_s02,X
            sta         tk_sc2,X
            jsr         u_to_a                              ;   t1: the end's tick
            lda         dl
            sta         tl_num
            lda         dl + 1
            sta         tl_num + 1
            jsr         a_add
            jsr         tick_at
            ldx         trk
            lda         m_at
            sta         tk_s10,X
            sec                                             ; Q = t1 - t0, 1 at least
            sbc         tk_s00,X
            sta         tk_sq_lo,X
            lda         m_at + 1
            sta         tk_s11,X
            sbc         tk_s01,X
            sta         tk_sq_hi,X
            lda         m_at + 2
            sta         tk_s12,X
            lda         tk_sq_lo,X
            ora         tk_sq_hi,X
            bne         :+
            inc         tk_sq_lo,X
:
            lda         #1
            sta         tk_sl,X
            rts

; The slide's next step into the FIFO (its key code and fraction at its tick); none left: the slide done.  A key off
; due before the step's tick goes first
slide_step:
            ldx         trk
            lda         tk_sc0,X                            ; t < t1?
            cmp         tk_s10,X
            lda         tk_sc1,X
            sbc         tk_s11,X
            lda         tk_sc2,X
            sbc         tk_s12,X
            bcc         :+
            stz         tk_sl,X
            rts
:
            sec                                             ; k = t - t0 (16 bits), kept on the stack
            lda         tk_sc0,X
            sbc         tk_s00,X
            sta         m_w
            pha
            lda         tk_sc1,X
            sbc         tk_s01,X
            sta         m_w + 1
            pha
            lda         tk_sd_lo,X                          ; k * dur divmod Q
            sta         tl_num
            lda         tk_sd_hi,X
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            jsr         mul32
            ldx         trk
            lda         tk_sq_lo,X
            sta         tl_den
            sta         m_q
            lda         tk_sq_hi,X
            sta         tl_den + 1
            sta         m_q + 1
            stz         tl_den + 2
            stz         tl_den + 3
            jsr         tl_div
            lda         tl_rem
            sta         m_rem
            lda         tl_rem + 1
            sta         m_rem + 1
            ldx         trk                                 ; m_a = u + the quotient
            clc
            lda         tk_su0,X
            adc         tl_num
            sta         m_a
            lda         tk_su1,X
            adc         tl_num + 1
            sta         m_a + 1
            lda         tk_su2,X
            adc         tl_num + 2
            sta         m_a + 2
            lda         tk_su3,X
            adc         tl_num + 3
            sta         m_a + 3
            jsr         tick_of                             ; m_at: the step's tick
            ldx         trk                                 ; A key off due before it: first
            lda         tk_koff,X
            beq         @step
            lda         tk_k0,X
            cmp         m_at
            lda         tk_k1,X
            sbc         m_at + 1
            lda         tk_k2,X
            sbc         m_at + 2
            bcs         @step
            lda         m_at
            pha
            lda         m_at + 1
            pha
            lda         m_at + 2
            pha
            jsr         koff_put
            pla
            sta         m_at + 2
            pla
            sta         m_at + 1
            pla
            sta         m_at
@step:
            pla                                             ; (k)
            sta         m_w + 1
            pla
            sta         m_w
            jsr         slide_pitch                         ; m_pv: the step's pitch
            jsr         kcf
            ldy         m_kc
            lda         trk
            ora         #$28
            jsr         put
            ldy         m_kf
            lda         trk
            ora         #$30
            jsr         put
            ldx         trk                                 ; t += 2
            clc
            lda         tk_sc0,X
            adc         #2
            sta         tk_sc0,X
            bcc         :+
            inc         tk_sc1,X
            bne         :+
            inc         tk_sc2,X
:
            rts

; m_pv = from + round ((to - from) * k / Q) (k in m_w, Q in m_q): floor ((2 (to - from) k + Q) / 2Q), signed: for a
; negative delta, -floor ((2 |delta| k + Q - 1) / 2Q)
slide_pitch:
            ldx         trk
            sec                                             ; Delta, its sign kept apart
            lda         tk_st_lo,X
            sbc         tk_sf_lo,X
            sta         tl_num
            lda         tk_st_hi,X
            sbc         tk_sf_hi,X
            sta         tl_num + 1
            stz         m_neg
            bpl         :+
            inc         m_neg
            sec
            lda         #0
            sbc         tl_num
            sta         tl_num
            lda         #0
            sbc         tl_num + 1
            sta         tl_num + 1
:
            stz         tl_num + 2
            stz         tl_num + 3
            jsr         mul32                               ; 2 |delta| k
            asl         tl_num
            rol         tl_num + 1
            rol         tl_num + 2
            rol         tl_num + 3
            clc                                             ; + Q
            lda         tl_num
            adc         m_q
            sta         tl_num
            lda         tl_num + 1
            adc         m_q + 1
            sta         tl_num + 1
            bcc         :+
            inc         tl_num + 2
            bne         :+
            inc         tl_num + 3
:
            lda         m_neg                               ; (- 1, for a negative delta: n + Q >= 1)
            beq         :++
            lda         tl_num
            bne         :+
            lda         tl_num + 1
            bne         :+
            dec         tl_num + 2
:
            lda         tl_num
            bne         :+
            dec         tl_num + 1
:
            lda         m_neg
            beq         :+
            dec         tl_num
:
            lda         m_q                                 ; / 2Q
            asl
            sta         tl_den
            lda         m_q + 1
            rol
            sta         tl_den + 1
            lda         #0
            rol
            sta         tl_den + 2
            stz         tl_den + 3
            jsr         tl_div
            ldx         trk
            lda         m_neg
            beq         :+
            sec                                             ; (Negative: the quotient's negation)
            lda         #0
            sbc         tl_num
            sta         tl_num
            lda         #0
            sbc         tl_num + 1
            sta         tl_num + 1
:
            clc
            lda         tk_sf_lo,X
            adc         tl_num
            sta         m_pv
            lda         tk_sf_hi,X
            adc         tl_num + 1
            sta         m_pv + 1
            rts

; The track's key off, due: into the FIFO at its tick
koff_put:
            ldx         trk
            stz         tk_koff,X
            lda         tk_k0,X
            sta         m_at
            lda         tk_k1,X
            sta         m_at + 1
            lda         tk_k2,X
            sta         m_at + 2
            ldy         trk
            lda         #$08
            jmp         put

; ****************************************************************************
; A track's MML, a command at a time

; Track trk's FIFO filled, if it can be: a slide's next step (a key off before it if it's due first), a key off, or
; the MML's next command, till there's a write.  OUT: C = 0, writes waiting; or C = 1: the track's done
tk_fill:
            ldx         trk
            lda         tk_fn,X
            beq         :+
            clc
            rts
:
            lda         tk_sl,X
            beq         :+
            jsr         slide_step
            bra         tk_fill
:
            lda         tk_koff,X
            beq         :+
            jsr         koff_put
            bra         tk_fill
:
            lda         tk_end,X
            beq         :+
            sec
            rts
:
            jsr         cur_get
            jsr         command
            jsr         cur_put
            bra         tk_fill

; The track's next command (cp at it)
command:
            jsr         ws
            jsr         getc
            bne         :+
            jmp         track_end
:
            sta         m_cmd
            ldx         #0                                  ; Which: the commands' characters
@find:
            ldy         cmd_chars,X
            beq         @what
            cmp         cmd_chars,X
            beq         @found
            inx
            bra         @find
@found:
            lda         cmd_hi,X
            pha
            lda         cmd_lo,X
            pha
            rts                                             ; (Its routine)
@what:
            LDR         r0, s_whatis
            jsr         msg_start
            lda         m_cmd
            jsr         msg_char
            jmp         track_msg

; The track's MML done: its time the longest's, if it's past it
track_end:
            ldx         trk
            lda         #1
            sta         tk_end,X
            lda         m_longest                           ; longest < u?
            cmp         tk_u0,X
            lda         m_longest + 1
            sbc         tk_u1,X
            lda         m_longest + 2
            sbc         tk_u2,X
            lda         m_longest + 3
            sbc         tk_u3,X
            bcs         :+
            lda         tk_u0,X
            sta         m_longest
            lda         tk_u1,X
            sta         m_longest + 1
            lda         tk_u2,X
            sta         m_longest + 2
            lda         tk_u3,X
            sta         m_longest + 3
:
            rts

; c-b: a note (+ or # sharp, - flat; a length): ((o + 1) * 12 + its semitone + k) * 64 + D
c_note:
            lda         m_cmd
            sec
            sbc         #'a'
            tax
            lda         semis,X
            sta         m_pv                                ; (The semitone, then the accidentals)
            stz         m_pv + 1
@acc:
            jsr         peek
            cmp         #'+'
            beq         @up
            cmp         #'#'
            beq         @up
            cmp         #'-'
            bne         @len
            jsr         getc
            lda         m_pv
            bne         :+
            dec         m_pv + 1
:
            dec         m_pv
            bra         @acc
@up:
            jsr         getc
            inc         m_pv
            bne         @acc
            inc         m_pv + 1
            bra         @acc
@len:
            jsr         len
            ldx         trk                                 ; An instrument there must be
            lda         tk_inst_hi,X
            ora         tk_pend_hi,X
            bne         :+
            LDR         r0, s_noinst
            jmp         track_fail
:
            lda         tk_oct,X                            ; (o + 1) * 12 (o signed) ...
            clc
            adc         #1
            sta         m_x
            lda         #0
            bit         m_x
            bpl         :+
            lda         #$FF
:
            sta         m_x + 1
            lda         #12
            sta         m_w
            stz         m_w + 1
            jsr         mul16
            clc                                             ;   + the semitone
            lda         m_x
            adc         m_pv
            sta         m_pv
            lda         m_x + 1
            adc         m_pv + 1
            sta         m_pv + 1
            ldx         trk                                 ;   + k
            lda         tk_tr,X
            jsr         pv_add_s
            ldx         #6                                  ;   * 64
:
            asl         m_pv
            rol         m_pv + 1
            dex
            bne         :-
            ldx         trk                                 ;   + D
            clc
            lda         m_pv
            adc         tk_det_lo,X
            sta         m_pv
            lda         m_pv + 1
            adc         tk_det_hi,X
            sta         m_pv + 1
            stz         m_drum
            jmp         play_note

; m_pv += .A (signed)
pv_add_s:
            ldy         #0
            cmp         #$80
            bcc         :+
            dey
:
            clc
            adc         m_pv
            sta         m_pv
            tya
            adc         m_pv + 1
            sta         m_pv + 1
            rts

; r: a rest
c_rest:
            jsr         len
            jmp         u_add

; x N: a General MIDI drum (the driver's patch and key code); the instrument @ named last for the notes after it
c_drum:
            jsr         num_must
            lda         nv + 1
            bne         @bad
            lda         nv
            cmp         #128
            bcs         @bad
            tax
            lda         drum_kc,X
            sta         m_dkc
            lda         drum_patch,X
            jsr         patch_of
            ldx         trk
            lda         mp
            sta         tk_pend_lo,X
            lda         mp + 1
            sta         tk_pend_hi,X
            stz         tk_slide,X
            stz         tk_legato,X
            jsr         len
            lda         #1
            sta         m_drum
            jsr         play_note
            ldx         trk
            stz         tk_inst_lo,X
            stz         tk_inst_hi,X
            lda         tk_name_lo,X
            sta         tk_pend_lo,X
            lda         tk_name_hi,X
            sta         tk_pend_hi,X
            rts
@bad:
            LDR         r0, s_nodrum
            jmp         track_fail

; o N, > <, l N, q N, k N, D N
c_oct:
            jsr         num_must
            lda         nv
            ldx         trk
            sta         tk_oct,X
            rts

c_up:
            ldx         trk
            inc         tk_oct,X
            rts

c_down:
            ldx         trk
            dec         tk_oct,X
            rts

c_l:
            jsr         num_must
            lda         nv
            ldx         trk
            sta         tk_len,X
            rts

c_q:
            jsr         num_must
            lda         nv
            ldx         trk
            sta         tk_gate,X
            rts

c_k:
            jsr         num_must
            lda         nv
            ldx         trk
            sta         tk_tr,X
            rts

c_detune:
            jsr         num_must
            ldx         trk
            lda         nv
            sta         tk_det_lo,X
            lda         nv + 1
            sta         tk_det_hi,X
            rts

; v N: the volume (the carriers' levels at once, if there's an instrument; its attenuation hysong.js's: 0 or
; less, 127; past 127, 127's)
c_vol:
            jsr         num_must
            lda         nv + 1
            bmi         @zero
            bne         @top
            lda         nv
            bpl         :+
@top:
            lda         #127
            bra         :+
@zero:
            lda         #0
:
            ldx         trk
            sta         tk_vol,X
            jmp         levels

; p l|r|c|0: the speakers (now, if there's an instrument)
c_pan:
            jsr         ws
            jsr         getc
            ldx         #3
:
            cmp         pan_chars,X
            beq         :+
            dex
            bpl         :-
            LDR         r0, s_badpan
            jmp         track_fail
:
            lda         pan_bits,X
            ldx         trk
            sta         tk_pan,X
            lda         tk_inst_hi,X
            bne         :+
            rts
:
            sta         mp + 1
            lda         tk_inst_lo,X
            sta         mp
            lda         (mp)
            and         #$3F
            ora         tk_pan,X
            tay
            lda         trk
            ora         #$20
            jmp         put_now

; @name: the instrument for the next note on
c_inst:
            ldy         #0                                  ; Its name: name characters, from here
:
            lda         (cp),Y
            jsr         is_word
            bcc         :+
            iny
            bne         :-
:
            cpy         #0
            beq         @none
            sty         m_len
            stz         m_r                                 ; Each instrument's name
@try:
            lda         m_r
            cmp         m_ninst
            bcs         @none
            jsr         inst_ptr
            ldy         #0
:
            lda         (m_x),Y
            beq         @end
            cmp         (cp),Y
            bne         @next
            iny
            bra         :-
@end:
            cpy         m_len
            beq         @found
@next:
            inc         m_r
            bra         @try
@found:
            tya                                             ; (cp past the name)
            clc
            adc         cp
            sta         cp
            bcc         :+
            inc         cp + 1
:
            clc                                             ; Its patch: past the name
            lda         m_x
            adc         #INST_NAME
            ldx         trk
            sta         tk_pend_lo,X
            sta         tk_name_lo,X
            lda         m_x + 1
            adc         #0
            sta         tk_pend_hi,X
            sta         tk_name_hi,X
            rts
@none:
            LDR         r0, s_unknown
            jmp         track_fail

; M pms,ams: the channel's LFO sensitivities
c_sens:
            jsr         num_must
            lda         nv
            and         #7
            asl
            asl
            asl
            asl
            pha
            jsr         getc                                ; (A character between: hysong.js's i++)
            jsr         num_must
            pla
            sta         m_reg
            lda         nv
            and         #3
            ora         m_reg
            ldx         trk
            sta         tk_lfo,X
            tay
            lda         #1
            sta         tk_lfoset,X
            lda         trk
            ora         #$38
            jmp         put_now

; L rate,pmd,amd,wave: the LFO
c_lfo:
            jsr         num_must
            ldy         nv
            lda         #$18
            jsr         put_now
            jsr         getc
            jsr         num_must
            lda         nv
            ora         #$80
            tay
            lda         #$19
            jsr         put_now
            jsr         getc
            jsr         num_must
            ldy         nv
            lda         #$19
            jsr         put_now
            jsr         getc
            jsr         num_must
            lda         nv
            and         #3
            tay
            lda         #$1B
            jmp         put_now

; N n, N-: the noise
c_noise:
            jsr         peek
            cmp         #'-'
            bne         :+
            jsr         getc
            ldy         #0
            bra         :++
:
            jsr         num_must
            lda         nv
            and         #31
            ora         #$80
            tay
:
            lda         #$0F
            jmp         put_now

; y reg,val: a register
c_reg:
            jsr         num_must
            lda         nv
            pha
            jsr         getc
            jsr         num_must
            ldy         nv
            pla
            jmp         put_now

; _ a slide to the next note, & legato, | a bar (nothing)
c_slide:
            ldx         trk
            lda         #1
            sta         tk_slide,X
            rts

c_legato:
            ldx         trk
            lda         #1
            sta         tk_legato,X
            rts

c_bar:
            rts

; [ : a repeat: its ] found (the track's lines followed, repeats inside counted), its count after it (none: 2); 0
; times: past it at once; else its start and end kept, and the count
c_repeat:
            MOVR        m_save, cp                          ; (The start: past the [)
            MOVR        m_savel, cl
            ldx         #0                                  ; (.X: repeats inside)
@scan:
            jsr         getc
            bne         :+
            LDR         r0, s_whatis                        ; (No ]: "what is ["?)
            jsr         msg_start
            lda         #'['
            jsr         msg_char
            jmp         track_msg
:
            cmp         #'['
            bne         :+
            inx
            bra         @scan
:
            cmp         #']'
            bne         @scan
            dex
            bpl         @scan
            jsr         count                               ; nv: the count
            lda         nv
            ora         nv + 1
            bne         :+
            rts                                             ; (0 times: cp past it)
:
            ldx         trk                                 ; Kept: start, end, count
            lda         tk_rp,X
            cmp         #RP_DEPTH
            bcc         :+
            LDR         r0, s_deep
            jmp         track_fail
:
            inc         tk_rp,X
            sta         m_r
            txa
            asl
            asl
            asl
            ora         m_r
            tay
            lda         m_save
            sta         rp_s_lo,Y
            lda         m_save + 1
            sta         rp_s_hi,Y
            lda         m_savel
            sta         rp_sl_lo,Y
            lda         m_savel + 1
            sta         rp_sl_hi,Y
            lda         cp
            sta         rp_e_lo,Y
            lda         cp + 1
            sta         rp_e_hi,Y
            lda         cl
            sta         rp_el_lo,Y
            lda         cl + 1
            sta         rp_el_hi,Y
            lda         nv                                  ; (Past 255: 255)
            ldx         nv + 1
            beq         :+
            lda         #255
:
            sta         rp_n,Y
            MOVR        cp, m_save
            MOVR        cl, m_savel
            rts

; The digits after a ] (none: 2).  OUT: nv
count:
            stz         nv
            stz         nv + 1
            ldx         #0
:
            jsr         peek
            sec
            sbc         #'0'
            cmp         #10
            bcs         :+
            jsr         nv_digit
            jsr         getc
            inx
            bra         :-
:
            cpx         #0
            bne         :+
            lda         #2
            sta         nv
:
            rts

; ] : the repeat's end: again from its start, or (its count done) on past its end
c_endrep:
            ldx         trk
            lda         tk_rp,X
            bne         :+
            LDR         r0, s_whatis
            jsr         msg_start
            lda         #']'
            jsr         msg_char
            jmp         track_msg
:
            dec         a
            sta         m_r
            txa
            asl
            asl
            asl
            ora         m_r
            tay
            lda         rp_n,Y
            dec         a
            sta         rp_n,Y
            beq         @done
            lda         rp_s_lo,Y
            sta         cp
            lda         rp_s_hi,Y
            sta         cp + 1
            lda         rp_sl_lo,Y
            sta         cl
            lda         rp_sl_hi,Y
            sta         cl + 1
            rts
@done:
            dec         tk_rp,X
            lda         rp_e_lo,Y
            sta         cp
            lda         rp_e_hi,Y
            sta         cp + 1
            lda         rp_el_lo,Y
            sta         cl
            lda         rp_el_hi,Y
            sta         cl + 1
            rts

; ****************************************************************************
; The stream: the tracks' writes merged, as ZSM bytes, into the ring

; The ring empty, and the score from its start
mml_start:
            stz         m_rh
            stz         m_rh + 1
            stz         m_rt
            stz         m_rt + 1
            stz         m_rn
            stz         m_rn + 1
            jmp         mml_reset

; The song's next step into the ring, if there's room for one and the song isn't done.  OUT: C = 1, no room or done
m_step:
            lda         m_done
            bne         @no
            sec                                             ; Room: RING - m_rn >= RING_ROOM
            lda         #<RING
            sbc         m_rn
            tax
            lda         #>RING
            sbc         m_rn + 1
            bne         :+
            cpx         #RING_ROOM
            bcc         @no
:
            jsr         m_next
            clc
            rts
@no:
            sec
            rts

; stage_read's, for a score: the ring's bytes into stage (256 at most), compiled first if it's empty; at the song's
; end, from its start again if it's to be played again (rloops), as a file is read on from its loop point
mml_stage:
            lda         staged
            ora         staged + 1
            ora         eof
            bne         @done
@fill:
            lda         m_rn                                ; Something there?
            ora         m_rn + 1
            bne         @move
            lda         m_done                              ; Empty: compiled now, or the end
            beq         @compile
            lda         rloops
            beq         @end
            cmp         #FOREVER
            beq         :+
            dec         rloops
:
            jsr         mml_reset
@compile:
            jsr         m_step
            bra         @fill
@end:
            lda         #1
            sta         eof
@done:
            rts
@move:
            ldx         #0
:
            lda         m_rn                                ; (Till the ring's empty, or 256)
            ora         m_rn + 1
            beq         :+
            jsr         ring_get
            sta         stage,X
            inx
            bne         :-
            inc         staged + 1
            rts
:
            stx         staged
            rts

; The ring's oldest byte.  OUT: .A.  Keeps .X, .Y
ring_get:
            phy
            clc
            lda         m_rh
            adc         #<m_ring
            sta         mp
            lda         m_rh + 1
            adc         #>m_ring
            sta         mp + 1
            lda         (mp)
            pha
            inc         m_rh
            bne         :+
            inc         m_rh + 1
            lda         m_rh + 1
            and         #>(RING - 1)
            sta         m_rh + 1
:
            lda         m_rn
            bne         :+
            dec         m_rn + 1
:
            dec         m_rn
            pla
            ply
            rts

; .A into the ring.  Keeps .X, .Y
rout:
            phy
            pha
            clc
            lda         m_rt
            adc         #<m_ring
            sta         mp
            lda         m_rt + 1
            adc         #>m_ring
            sta         mp + 1
            pla
            sta         (mp)
            inc         m_rt
            bne         :+
            inc         m_rt + 1
            pha
            lda         m_rt + 1
            and         #>(RING - 1)
            sta         m_rt + 1
            pla
:
            inc         m_rn
            bne         :+
            inc         m_rn + 1
:
            ply
            rts

; As the player waits for its next tick: steps compiled, till the ring's full or the song's done, or the system's
; tick reaches m_tick (the next song tick's: a step's lateness at most)
mml_idle:
            jsr         TICKS
            sec
            sbc         m_tick
            txa
            sbc         m_tick + 1
            bpl         @done                               ; (Reached: now - m_tick >= 0)
            jsr         m_step
            bcc         mml_idle
@done:
            rts

; The song's next step into the ring: the tracks' FIFOs' earliest write (the lowest track's of those at the same
; tick); if its tick is past now, the waiting pairs out and a delay toward it (127 ticks at most) instead.  None
; left: the end (every channel keyed off at the longest track's time, then $80)
m_next:
            lda         m_ending
            beq         :+
            jmp         @ending
:
            lda         #$FF
            sta         m_best
            stz         trk
@track:
            jsr         tk_fill
            bcs         @next
            ldx         trk                                 ; Its head's place
            txa
            asl
            asl
            asl
            asl
            asl
            ora         tk_fh,X
            tax
            ldy         m_best                              ; Earlier than the best so far?
            bmi         @take
            lda         fi_t0,X
            cmp         m_bt
            lda         fi_t1,X
            sbc         m_bt + 1
            lda         fi_t2,X
            sbc         m_bt + 2
            bcs         @next
@take:
            lda         fi_t0,X
            sta         m_bt
            lda         fi_t1,X
            sta         m_bt + 1
            lda         fi_t2,X
            sta         m_bt + 2
            lda         trk
            sta         m_best
@next:
            inc         trk
            lda         trk
            cmp         #TRACKS
            bcc         @track
            ldx         m_best
            bmi         @last
            lda         m_bt
            sta         m_at
            lda         m_bt + 1
            sta         m_at + 1
            lda         m_bt + 2
            sta         m_at + 2
            jsr         delay_to                            ; Its time yet?
            bcs         @out
            ldx         m_best                              ; Taken from its FIFO
            txa
            asl
            asl
            asl
            asl
            asl
            ora         tk_fh,X
            tay
            lda         tk_fh,X
            inc         a
            and         #FIFO - 1
            sta         tk_fh,X
            dec         tk_fn,X
            lda         fi_val,Y
            pha
            lda         fi_reg,Y
            ply
            jmp         emit
@out:
            rts

@last:                                                      ; Every track done: the end's tick
            ldx         #3
:
            lda         m_longest,X
            sta         m_a,X
            dex
            bpl         :-
            jsr         tick_at
            lda         m_at
            sta         m_endt
            lda         m_at + 1
            sta         m_endt + 1
            lda         m_at + 2
            sta         m_endt + 2
            lda         #1
            sta         m_ending
@ending:
            lda         m_endt
            sta         m_at
            lda         m_endt + 1
            sta         m_at + 1
            lda         m_endt + 2
            sta         m_at + 2
            jsr         delay_to
            bcs         @out
            stz         m_r                                 ; Every channel keyed off
:
            ldy         m_r
            lda         #$08
            jsr         emit
            inc         m_r
            lda         m_r
            cmp         #TRACKS
            bcc         :-
            jsr         flush_pairs
            lda         #$80
            jsr         rout
            lda         #1
            sta         m_done
            rts

; m_at past now?  The waiting pairs out, and a delay toward it (127 ticks at most).  OUT: C = 1 a delay went out;
; C = 0 it's now
delay_to:
            sec                                             ; d = at - now
            lda         m_at
            sbc         m_now
            sta         m_x
            lda         m_at + 1
            sbc         m_now + 1
            sta         m_x + 1
            lda         m_at + 2
            sbc         m_now + 2
            ora         m_x + 1
            bne         @most
            lda         m_x
            bne         :+
            clc
            rts
:
            cmp         #127
            bcc         :+
@most:
            lda         #127
:
            pha
            jsr         flush_pairs
            pla
            pha
            ora         #$80
            jsr         rout
            pla
            clc
            adc         m_now
            sta         m_now
            bcc         :+
            inc         m_now + 1
            bne         :+
            inc         m_now + 2
:
            sec
            rts

; Register .A = .Y into the stream's pairs (now's): the same value as the register's last left out (but $08, $01,
; $19); 63 pairs at most in a chunk
emit:
            sta         m_er
            sty         m_ev
            cmp         #$08
            beq         @put
            cmp         #$01
            beq         @put
            cmp         #$19
            beq         @put
            lsr                                             ; Written before, the same?
            lsr
            lsr
            tax
            lda         m_er
            and         #7
            tay
            lda         bits,Y
            sta         m_bit
            and         m_known,X
            beq         @new
            ldy         m_er
            lda         m_shadow,Y
            cmp         m_ev
            bne         @put
            rts
@new:
            lda         m_bit
            ora         m_known,X
            sta         m_known,X
@put:
            ldx         m_er
            lda         m_ev
            sta         m_shadow,X
            ldy         m_np
            lda         m_er
            sta         m_pairs,Y
            lda         m_ev
            sta         m_pairs + 1,Y
            iny
            iny
            sty         m_np
            cpy         #126
            bcs         flush_pairs
            rts

; The waiting pairs into the stream: $40 + their count, then them
flush_pairs:
            lda         m_np
            beq         @done
            lsr
            ora         #$40
            jsr         rout
            ldx         #0
:
            lda         m_pairs,X
            jsr         rout
            inx
            cpx         m_np
            bcc         :-
            stz         m_np
@done:
            rts

; ****************************************************************************
; -o: the score compiled into a ZSM file (its name the argument after the score's)

mml_out:
            MOVR        r0, tl_arg                          ; The file, made (or emptied)
            lda         #O_WRITE | O_TRUNC
            ldx         #0
            jsr         CREATE
            bcc         @made
            cmp         #E_EXIST
            bne         @fail
            MOVR        r0, tl_arg
            lda         #O_WRITE | O_TRUNC
            jsr         OPEN
            bcc         @made
@fail:
            pha
            MOVR        r0, tl_arg
            pla
            jsr         tl_err
            jmp         tl_end
@made:
            sta         snd                                 ; (Its fd: snd's place)
            ldx         #HDR_SIZE - 1                       ; The header: "zm", 1, no loop, the channels, the rate
:
            stz         hdr,X
            dex
            bpl         :-
            lda         #'z'
            sta         hdr
            lda         #'m'
            sta         hdr + 1
            lda         #1
            sta         hdr + 2
            lda         m_mask
            sta         hdr + H_FM
            lda         m_rate
            sta         hdr + H_RATE
            lda         m_rate + 1
            sta         hdr + H_RATE + 1
            LDR         r0, hdr
            LDR         r1, HDR_SIZE
            jsr         @write
            stz         rloops
            stz         eof
            jsr         mml_start
@stage:
            stz         staged
            stz         staged + 1
            jsr         mml_stage
            lda         staged
            ora         staged + 1
            beq         @end
            LDR         r0, stage
            MOVR        r1, staged
            jsr         @write
            bra         @stage
@end:
            jmp         tl_end
@write:
            lda         snd
            jsr         WRITE
            bcc         :+
            jmp         @fail
:
            rts

; ****************************************************************************
; Errors

; "channel N: " and the message at r0 (fail's: "play: song: channel N: ...")
track_fail:
            MOVR        m_save, r0
            ldy         #0
            sty         m_r
            LDR         r0, s_channel
            jsr         msg_add
            lda         trk
            ora         #'0'
            jsr         msg_char
            lda         #':'
            jsr         msg_char
            lda         #' '
            jsr         msg_char
            MOVR        r0, m_save
            jsr         msg_add
            jmp         fail_msg

; The track's message in m_msg (msg_start's ...): "channel N: " before it, and fail
track_msg:
            lda         #0
            jsr         msg_char
            ldx         #63                                 ; (Moved along for "channel N: ")
:
            lda         m_msg - 11,X
            sta         m_msg,X
            dex
            cpx         #11
            bcs         :-
            ldy         #0
:
            lda         s_channel,Y
            beq         :+
            sta         m_msg,Y
            iny
            bra         :-
:
            lda         trk
            ora         #'0'
            sta         m_msg,Y
            lda         #':'
            sta         m_msg + 1,Y
            lda         #' '
            sta         m_msg + 2,Y
            LDR         r0, m_msg
            jmp         fail

; The message at r0 started in m_msg (msg_add: added); msg_char adds .A; fail_msg ends it and fails
msg_start:
            stz         m_r
msg_add:
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            phy
            jsr         msg_char
            ply
            iny
            bra         :-
:
            rts

msg_char:
            ldy         m_r
            cpy         #63
            bcs         :+
            sta         m_msg,Y
            inc         m_r
:
            rts

fail_msg:
            ldy         m_r
            lda         #0
            sta         m_msg,Y
            LDR         r0, m_msg
            jmp         fail

; The system's error .A, about the song
fail_os:
            pha
            MOVR        r0, song
            pla
            jsr         tl_err
            jmp         tl_end

.rodata
s_mml:      .byte       ".mml"
s_big:      .byte       "too big a score", 0
s_toomany:  .byte       "too many instruments", 0
s_badinst:  .byte       "an instrument it can't read", 0
s_what:     .byte       "what is this line?", 0
s_hash:     .byte       "a # line it doesn't know", 0
s_nonum:    .byte       "a number wanted", 0
s_badlen:   .byte       "a length it can't play", 0
s_toomuch:  .byte       "too much at once", 0
s_noinst:   .byte       "a note before an instrument", 0
s_nodrum:   .byte       "no such drum", 0
s_badpan:   .byte       "p takes l, r, c or 0", 0
s_unknown:  .byte       "no such instrument", 0
s_deep:     .byte       "repeats too deep", 0
s_whatis:   .byte       "what is ", 0
s_channel:  .byte       "channel ", 0
s_tempo:    .byte       "tempo", 0
s_rate:     .byte       "rate", 0
s_title:    .byte       "title", 0
hash_lo:    .lobytes    s_tempo, s_rate, s_title, 0
hash_hi:    .hibytes    s_tempo, s_rate, s_title, 0
bits:       .byte       $01, $02, $04, $08, $10, $20, $40, $80
; The carriers by algorithm (a bit each: M1 1, M2 2, C1 4, C2 8)
carriers:   .byte       $08, $08, $08, $08, $0C, $0E, $0E, $0F
reg_rows:   .byte       $40, $60, $80, $A0, $C0, $E0
note_codes: .byte       0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14
semis:      .byte       9, 11, 0, 2, 4, 5, 7                ; a b c d e f g
pan_chars:  .byte       "lrc0"
pan_bits:   .byte       $40, $80, $C0, $00
; The MML's commands and their routines (- 1, for rts)
cmd_chars:  .byte       "cdefgabrxo><lqvp@kDMLNy_&|[]", 0
cmd_lo:     .lobytes    c_note - 1, c_note - 1, c_note - 1, c_note - 1, c_note - 1, c_note - 1, c_note - 1, c_rest - 1
            .lobytes    c_drum - 1, c_oct - 1, c_up - 1, c_down - 1, c_l - 1, c_q - 1, c_vol - 1, c_pan - 1
            .lobytes    c_inst - 1, c_k - 1, c_detune - 1, c_sens - 1, c_lfo - 1, c_noise - 1, c_reg - 1
            .lobytes    c_slide - 1, c_legato - 1, c_bar - 1, c_repeat - 1, c_endrep - 1
cmd_hi:     .hibytes    c_note - 1, c_note - 1, c_note - 1, c_note - 1, c_note - 1, c_note - 1, c_note - 1, c_rest - 1
            .hibytes    c_drum - 1, c_oct - 1, c_up - 1, c_down - 1, c_l - 1, c_q - 1, c_vol - 1, c_pan - 1
            .hibytes    c_inst - 1, c_k - 1, c_detune - 1, c_sens - 1, c_lfo - 1, c_noise - 1, c_reg - 1
            .hibytes    c_slide - 1, c_legato - 1, c_bar - 1, c_repeat - 1, c_endrep - 1
; An instrument's words: gm, the voice's, the operators, the operators' (each: its patch byte, or $80 + its row's
; first, the operator added; its field's mask and shift)
V_GM        = 0
V_M1        = 5
V_C2        = 8
s_gm:       .byte       "gm", 0
s_alg:      .byte       "alg", 0
s_fb:       .byte       "fb", 0
s_pms:      .byte       "pms", 0
s_ams:      .byte       "ams", 0
s_m1:       .byte       "m1", 0
s_m2:       .byte       "m2", 0
s_c1:       .byte       "c1", 0
s_c2:       .byte       "c2", 0
s_mul:      .byte       "mul", 0
s_dt1:      .byte       "dt1", 0
s_dt2:      .byte       "dt2", 0
s_tl:       .byte       "tl", 0
s_ks:       .byte       "ks", 0
s_ar:       .byte       "ar", 0
s_d1r:      .byte       "d1r", 0
s_d2r:      .byte       "d2r", 0
s_d1l:      .byte       "d1l", 0
s_rr:       .byte       "rr", 0
s_am:       .byte       "am", 0
voice_lo:   .lobytes    s_gm, s_alg, s_fb, s_pms, s_ams, s_m1, s_m2, s_c1, s_c2, s_mul, s_dt1, s_dt2, s_tl, s_ks
            .lobytes    s_ar, s_d1r, s_d2r, s_d1l, s_rr, s_am, 0
voice_hi:   .hibytes    s_gm, s_alg, s_fb, s_pms, s_ams, s_m1, s_m2, s_c1, s_c2, s_mul, s_dt1, s_dt2, s_tl, s_ks
            .hibytes    s_ar, s_d1r, s_d2r, s_d1l, s_rr, s_am, 0
; (The patch's bytes from INST_NAME: 0 $20 (FB, CON), 1 $38 (PMS, AMS), 2-5 DT1/MUL, 6-9 TL, 10-13 KS/AR, 14-17
; AM/D1R, 18-21 DT2/D2R, 22-25 D1L/RR)
voice_reg:  .byte       0, INST_NAME, INST_NAME, INST_NAME + 1, INST_NAME + 1, 0, 0, 0, 0
            .byte       $80 + INST_NAME + 2, $80 + INST_NAME + 2, $80 + INST_NAME + 18, $80 + INST_NAME + 6
            .byte       $80 + INST_NAME + 10, $80 + INST_NAME + 10, $80 + INST_NAME + 14, $80 + INST_NAME + 18
            .byte       $80 + INST_NAME + 22, $80 + INST_NAME + 22, $80 + INST_NAME + 14
voice_mask: .byte       0, $07, $38, $70, $03, 0, 0, 0, 0
            .byte       $0F, $70, $C0, $7F, $C0, $1F, $1F, $1F, $F0, $0F, $80
voice_shift: .byte      0, 0, 3, 4, 0, 0, 0, 0, 0
            .byte       0, 4, 6, 0, 6, 0, 0, 0, 4, 0, 7
.code
