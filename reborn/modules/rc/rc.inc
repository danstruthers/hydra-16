; ****************************************************************************
; rc.inc - rc's constants, and what its objects share (each includes it: a name .global here is one object's, used
; by others).  See rc.s.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

; ---- Tokens (lex.s: tok)
T_EOF           = 0
T_NL            = 1             ; A new line
T_SEMI          = 2             ; ;
T_AMP           = 3             ; &
T_ANDAND        = 4             ; &&
T_OROR          = 5             ; ||
T_PIPE          = 6             ; | (tok_fd: the left's fd, 1; or |[n])
T_LBRACE        = 7
T_RBRACE        = 8
T_LPAREN        = 9
T_RPAREN        = 10
T_CARET         = 11            ; ^
T_DOLLAR        = 12            ; $ (tok_text: the name)
T_COUNT         = 13            ; $# (tok_text: the name)
T_FLAT          = 14            ; $" (tok_text: the name)
T_BQ            = 15            ; `
T_REDIR         = 16            ; < > >> (tok_rtype, tok_fd; tok_fd2 for >[n=m])
T_WORD          = 17            ; A word's text (tok_text, tok_len; tok_quoted: some of it was quoted)
T_SUB           = 18            ; ( straight after a $name: its subscripts
T_EQ            = 19            ; =
T_IF            = 20            ; The keywords, where a command starts
T_NOT           = 21
T_FOR           = 22
T_IN            = 23
T_WHILE         = 24
T_SWITCH        = 25
T_CASE          = 26
T_FN            = 27
T_TWIDDLE       = 28            ; ~
T_BANG          = 29            ; !

; ---- Redirections (tok_rtype, a redirection node's type)
RD_IN           = 1             ; <
RD_OUT          = 2             ; >
RD_APPEND       = 3             ; >>
RD_DUP          = 4             ; >[n=m]
RD_CLOSE        = 5             ; >[n=]

; ---- Glob markers: unquoted * ? [ in a word's text (lex.s); a pattern's, till globbing
G_STAR          = $01
G_QUERY         = $02
G_BRACKET       = $03

; ---- The tree (parse.s): a command node's type, then its fields (pointers: 0 for none)
C_SIMPLE        = 1             ; +1 words (a word list), +3 redirections, +5 assignments
C_BRACE         = 2             ; +1 body, +3 redirections
C_SEQ           = 3             ; +1 the first, +3 the next
C_BG            = 4             ; +1 the command, +3 its text, +5 its text's end
C_AND           = 5             ; +1, +3
C_OR            = 6             ; +1, +3
C_NOT           = 7             ; +1
C_PIPE          = 8             ; +1 the left, +3 the right, +5 the left's fd, +6 the left's text, +8 its end, +10 the
                                ;   right's text, +12 its end
C_IF            = 9             ; +1 the condition, +3 the command
C_IFNOT         = 10            ; +1 the command
C_FOR           = 11            ; +1 the variable's name (a word), +3 the list (a word list), +5 the body, +7 <> 0: a
                                ;   list given (else $*)
C_WHILE         = 12            ; +1 the condition, +3 the body
C_SWITCH        = 13            ; +1 the subject (a word), +3 the body (its cases and commands)
C_CASE          = 14            ; +1 the patterns (a word list)
C_MATCH         = 15            ; +1 the subject, +3 the patterns
C_FN            = 16            ; +1 the names (a word list), +3 the body's text, +5 its end
C_FNDEL         = 17            ; +1 the names
; Word nodes
W_LIT           = 1             ; +1 the length, +2 the text (glob markers in it)
W_VAR           = 2             ; +1 the name (a word), +3 the subscripts (a word list), or 0
W_COUNT         = 3             ; +1 the name
W_FLAT          = 4             ; +1 the name
W_CAT           = 5             ; +1, +3: the two words
W_LIST          = 6             ; +1 the words (a word list)
W_BQ            = 7             ; +1 the command, +3 its text, +5 its end
; A word list: +0 the next, +2 the word.  A redirection: +0 the next, +2 its type (RD_*), +3 its fd, +4 the other
; fd (RD_DUP), +5 its file (a word).  An assignment: +0 the next, +2 the name (a word), +4 the value (a word)

; ---- Lists of words (word.s, var.s): each word its length (0-254), then its bytes; $FF after the last
LIST_END        = $FF
WORD_MAX        = 254

; ---- Sizes
TOK_MAX         = 255           ; A word's text, at most
LINE_SIZE       = 256           ; An input buffer
TEXT_SIZE       = 2048          ; The commands' text being parsed (from files and the console)
ARENA_SIZE      = 10240         ; The commands' trees, and what expanding them makes
HEAP_SIZE       = 8192          ; Variables and functions
SRC_MAX         = 8             ; Input sources, nested (., eval, functions)
SAVE_MAX        = 16            ; Fds saved by redirections
BANG_MAX        = 63            ; A script's first line read (#! and its interpreter's path: PATH_MAX at most)
RC_FD_HIGH      = 10            ; rc's own fds (scripts', saved ones) from here: a command gets 0-9
CHILD_FDS       = 10

; ---- Variables (var.s): a record in the heap: +0 the next, +2 flags (VF_*), +3 the value (a list in the heap, a
; function's text: its length (2) and it), +5 the name's length, +6 the name
VF_FN           = $01           ; A function
VF_DIRTY        = $02           ; Changed since it went to the environment
VF_GONE         = $04           ; Removed: to go from the environment too

; ---- Push and pull a zero-page word
.macro PUSHW    var
            lda         var
            pha
            lda         var + 1
            pha
.endmacro

.macro PULLW    var
            pla
            sta         var + 1
            pla
            sta         var
.endmacro

; ---- What each object gives (lex.s and parse.s: the second bank's: called by FAR2, and they call the first's by
; FAR1)
; rc.s (and its header's trampolines between the banks: hyx2.inc)
.globalzp   hyx2_to, hyx2_a
.global     hyx2_far1, hyx2_far2
.globalzp   ap, ctop, src_at, p0, p1, p2, p3
.global     src, t0, t1, num
.global     rc_error, rc_say, rc_say2, say_error, rc_abort, rc_true, rc_number, out2, out2s, interrupted, s_rc_colon
.global     src_getc, run_text, run_file
; mem.s
.global     arena_alloc, arena_mark, arena_release, heap_alloc, heap_free, mem_init, str_len, list_count, list_next
.global     list_copy_heap, list_alloc_word, list_end, list_start, list_append_word, arena_base, heap_base
; lex.s
.global     tok, tok_len, tok_fd, tok_fd2, tok_rtype, tok_quoted, tok_adj, tok_start, tok_kw
.global     lex, tok_text, lex_init, lex_peek, lex_next, lex_flush, lex_pos
; parse.s
.global     parse_line, parse_text
; word.s
.globalzp   wa
.global     expand, expand_list, expand_raw_list, expand_one, expand_one_list, match, glob_strip, list_flat, list_dup
; exec.s
.globalzp   xw
.global     xf
.global     exec_init, run_cmd, run_capture, spawn_path, wait_child, wait_bg, run_bg_reap, rc_exits
.global     redir_restore_all, status_word, status_error, high_fd, high_close
; var.s
.globalzp   vr
.global     var_find, var_fn, var_get, var_set, var_set_word, fn_set, fn_del, var_export, var_import
.global     var_local_mark, var_local_save, var_local_set, var_local_restore
; builtin.s (and nslib.s, included there)
.global     builtin_find, builtin_run, ns_default
