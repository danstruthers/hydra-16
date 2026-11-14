; ****************************************************************************
; lnzp.inc - the contract between the game's library (modules/dq0lib: the paged ROM's) and each task that reads the
; chunks (the world, the denizens, the senses) for the squares' code there (ENGINE-CRITIQUE.md's §4, step 2,
; 2026-11-14): the chunk lookup (AT_FIND, CK_MADE), the one line (LN_LINE, LN_PASS, LN_PORTAL) and the one way-finder
; (WK_ROW, WK_SEED, WK_SEARCH).  Assembled by the library and by the tasks (games/dq0w/at.s, line.inc, walk.h's C).
;
; A library runs in its caller's task with no RAM of its own: what it keeps over a call back into the task (at ()'s
; long way may ask the maker for a chunk, C that changes the call registers) is a block of the task's zero page, here,
; fixed (the program's zero page is $22-$7F, which the OS never touches; cc65's and each task's own end at $43: the
; tasks' cfgs end their ZP at LZ), and the task's chunk state (struct ats, its RAM: at ()'s quick way's and the
; long way's, the task's at.s's), which the block points to.  The task's own code the library calls (by the vectors
; here and in ats): hub_map (a hub's bank in the window), the maker asked for a chunk (ask_maker's), the line's
; ln_way (the world's: someone in the way), the search's square reached (the world's: its time, a trap).

LZ          = $44                       ; The block: $44-$70 (LZ_END)

; Kept between calls (set as the task starts, chunk.c's chunks_init, or by its C as line.h and walk.h say)
LZ_ATS      = LZ + 0                    ; (2) The task's struct ats
LZ_MAP      = LZ + 2                    ; (2) Its hub_map (__fastcall__: .A the bank)
LZ_WAY      = LZ + 4                    ; (2) line.h's ln_way: asked of each square the line passes (.A ln_n: not
                                        ;   0, it stopped there), or 0
LZ_EYE      = LZ + 6                    ; line.h's ln_eye (the eyes, feet above the first square's floor)
LZ_N        = LZ + 7                    ; line.h's ln_n, ln_first, ln_last (line's: the squares it went through,
LZ_FIRST    = LZ + 8                    ;   the first one's ln_pass and the last one's)
LZ_LAST     = LZ + 9

; A line's (LN_LINE, LN_PASS, LN_PORTAL: the task's stubs put its arguments here)
LZ_XY       = LZ + 10                   ; (8) The last square (ty, tx), the square it's at (y, x: line.h's ln_y, ln_x)
LZ_LY       = LZ_XY + 4
LZ_LX       = LZ_XY + 6
LZ_SN       = LZ + 18                   ; The stratum
LZ_MODE     = LZ + 19                   ; The mode (line.h's LN_*)
LZ_LP       = LZ + 20                   ; (2) (Its work: the square's pointer, ...)
LZ_AX8      = LZ + 22
LZ_AY8      = LZ + 23
LZ_SX       = LZ + 24
LZ_SY       = LZ + 25
LZ_NS       = LZ + 26
LZ_E        = LZ + 27                   ; (2)
LZ_E2       = LZ + 29                   ; (2)
LZ_AIR      = LZ + 31
LZ_LZ       = LZ + 32                   ; (2)
LZ_AD       = LZ + 34                   ; (2)
LZ_NEG      = LZ + 36
LZ_ACC      = LZ + 37                   ; (2)
LZ_KX       = LZ + 39                   ; (2) (LN_CACHED's: the chunk asked of the cache last, ...)
LZ_KY       = LZ + 41                   ; (2)
LZ_KOK      = LZ + 43
LZ_OK       = LZ + 44
LZ_END      = LZ + 45

; A search's (WK_ROW, WK_SEED, WK_SEARCH: the same bytes as a line's work, never both at once; walk.h's names)
LZ_CL       = LZ + 10                   ; (2) The grid's cells (CW * CW: walk.h's WC_*)
LZ_EL       = LZ + 12                   ; (2) Each square's floor (signed char[CW * CW])
LZ_RING     = LZ + 14                   ; (2) The queue (unsigned[WK_RN])
LZ_RULES    = LZ + 16                   ; The walker's rules (walk.h's WK_HANDS, WK_NODOOR)
LZ_X0       = LZ + 17                   ; The box searched
LZ_Y0       = LZ + 18
LZ_W        = LZ + 19
LZ_H        = LZ + 20
LZ_QH       = LZ + 21                   ; (2) The queue's head and tail
LZ_QT       = LZ + 23                   ; (2)
LZ_STOP     = LZ + 25                   ; (2) The square it stops at ($FFFF none)
LZ_REACH    = LZ + 27                   ; (2) Asked of each square reached (the world's), or 0
LZ_U        = LZ + 29                   ; (2) The square reached from (WK_ROW: the row's squares' kinds),
LZ_V        = LZ + 31                   ; (2)   the square reached (WK_SEED: the one queued; WK_ROW: their doors),
LZ_DIR      = LZ + 33                   ;   the way it's come by (0-7; WK_ROW: the row's squares),
LZ_DZ       = LZ + 34                   ;   its floor above u's,
LZ_V8       = LZ + 35                   ;   its cell, before it's reached
LZ_UX       = LZ + 36                   ; (The search's work)
LZ_UY       = LZ + 37
LZ_U8       = LZ + 38
LZ_EU       = LZ + 39
LZ_VX       = LZ + 40
LZ_VY       = LZ + 41
LZ_C        = LZ + 42
LZ_I        = LZ + 43
LZ_LIM      = LZ + 44
.assert     LZ_LIM < LZ_END, error, "lnzp.inc: the search's bytes past the block"
.assert     LZ_END <= $80, error, "lnzp.inc: the block past the program's zero page"

; struct ats: a task's chunk state (its RAM, the task's at.s's): the vectors and the tables it gives the library,
; at ()'s quick way (the chunk found last), the long way's own (the cache's slot found last), and world.h's C, cx_, cy_
ATS_ASK     = 0                         ; (2) The maker asked for chunk r1, r2 of stratum r0 (ask_maker (M_CHUNK ...))
ATS_STRATA  = 2                         ; (2) world.h's strata
ATS_KINDS   = 4                         ; (2) slant.s's t_kinds (world.h's T_PASS) and cornk (a diagonal's corner by its
ATS_CORNK   = 6                         ; (2)   kind)
ATS_DIRX    = 8                         ; (2) level.c's dir_x, dir_y
ATS_DIRY    = 10                        ; (2)
ATS_ME      = 12                        ; ASK_* (the slot the task has in use: the world bank's inuse)
ATS_SN      = 13                        ; at ()'s quick way: the stratum found last ($FF: none), its bounds, the chunk's
ATS_X0      = 14                        ; (2)   first square in it, its bank, the chunk, its squares
ATS_Y0      = 16                        ; (2)
ATS_W       = 18                        ; (2)
ATS_H       = 20                        ; (2)
ATS_OX      = 22                        ; (2)
ATS_OY      = 24                        ; (2)
ATS_BANK    = 26
ATS_CHUNK   = 27                        ; (2)
ATS_TILE    = 29                        ; (2)
ATS_C       = 31                        ; (2) world.h's C, cx_, cy_ (the chunk in the window, the square in it)
ATS_CX      = 33
ATS_CY      = 34
ATS_LASTK   = 35                        ; The long way's: the cache's slot found last ($FF none), what it holds,
ATS_LSN     = 36                        ;   its bank and its chunk in the window
ATS_LCX     = 37                        ; (2)
ATS_LCY     = 39                        ; (2)
ATS_LBANK   = 41
ATS_LCHUNK  = 42                        ; (2)
ATS_SIZE    = 44

; The library's entries (their low bytes: dq0lib.s's jump table)
AT_FIND     = $4B                       ; at ()'s long way: r0 sn, r1 x, r2 y; .A .X the square (0: none)
CK_MADE     = $4E                       ; Chunk r1, r2 of stratum r0 in the cache (made): .A 1, else 0 (no stamp)
CK_AT       = $51                       ; Square r1, r2 of stratum r0's chunk in the cache: .A 1, else 0
LN_LINE     = $54                       ; line.h's line: LZ_XY, LZ_SN, LZ_MODE; .A (.X 0)
LN_PASS     = $57                       ; line.h's ln_pass: LZ_LY, LZ_LX, LZ_SN, LZ_MODE; .A (.X 0)
LN_PORTAL   = $5A                       ; line.h's ln_portal: r0 the portal, LZ_MODE; .A (.X 0)
WK_ROW      = $5D                       ; walk.h's: a row's squares' cells (LZ_U the kinds, made their cells; LZ_V
                                        ;   their doors, WD_*; LZ_DIR how many), by LZ_RULES
WK_SEED     = $60                       ; walk.h's: square LZ_V queued, a seed
WK_SEARCH   = $63                       ; walk.h's: the search, from the squares queued
