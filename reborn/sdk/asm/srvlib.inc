; ****************************************************************************
; srvlib.inc - the server library's constants and its tree macro, included at the top of a server's source (the
; code, srvlib.s, at its end).  See srvlib.s.

; ---- The tree's entries
SE_NAME         = 0
SE_PARENT       = 2
SE_KIND         = 3
SE_HANDLER      = 4
SE_MODE         = 6
SE_AUX          = 7
SE_SIZE         = 8
SK_DIR          = 1
SK_TEXT         = 2
SK_CTL          = 3
SK_DATA         = 4
SK_DYN          = 5
SE_TEMPLATE     = $FD           ; SE_PARENT of a dynamic directory's template (its children are it)
DYN_NAME        = 0             ; An SK_DYN handler's requests: the srv_k-th child's id and name ...
DYN_FIND        = 1             ;   the child named at srv_p ...
DYN_IDNAME      = 2             ;   and child .X's name
SRV_DNAME_MAX   = 15            ; A dynamic child's name's length at most
SM_READ         = $04           ; (As Unix's other-permission bits: read, write)
SM_WRITE        = $02
SC_NAME         = 0             ; A ctl command: its word ...
SC_HANDLER      = 2             ;   and its handler
SC_SIZE         = 4
SRV_FIDS        = 32            ; Fids open at once
SRV_TEXT_MAX    = 255           ; A text file's length at most
SRV_ARGS        = 4             ; A ctl command's words after it, at most
SRV_CTL_MAX     = 63            ; A ctl write's length at most

; An entry of srv_tree: name, parent, kind, handler, mode, aux
.macro SRV_ENTRY name, parent, kind, handler, mode, aux
            .word       name
            .byte       parent, kind
            .word       handler
            .byte       mode, aux
.endmacro
