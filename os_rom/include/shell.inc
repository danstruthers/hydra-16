.debuginfo

; ****************************************************************************
; The shell (BIOS ROM page 7: shell/; HyForth's words call it): SH_CMD's commands (.X = SHC_*)

SHC_LS          = 0             ; .A.Y = a directory or file (.Y = 0: the current directory): shown
SHC_CAT         = 2             ; .A.Y = a file: shown
SHC_RM          = 4             ; .A.Y = a file: removed (not a directory)
SHC_RMDIR       = 6             ; .A.Y = an empty directory: removed
SHC_MKDIR       = 8             ; .A.Y = a directory: made
SHC_CP          = 10            ; SHBUF2 = a file, .A.Y = its copy (or a directory to copy it into)
SHC_MV          = 12            ; SHBUF2 = a file, .A.Y = its new name (in its directory), or where it moves to
SHC_RUN         = 14            ; .A.Y = a program: run (run.s).  OUT: .A = 0, it ran; or .A = SH_RUN_FD, a script
SHC_EXEC        = 16            ; .A.Y = a program's name (no .hyx or .hys): found (run.s), then run as SHC_RUN does
SHC_EDIT        = 18            ; .A.Y = a file (.Y = 0: none yet): the editor on it (page 8), waited for
SHC_VOLS        = 20            ; The cards: each one's details
SHC_MKFS        = 22            ; .A = a card, SHBUF2 = a label: an empty HydraFS on it
SHC_RELABEL     = 24            ; .A = a card, SHBUF2 = a label: the new label
SHC_FSCK        = 26            ; .A = a card: its HydraFS checked
SHC_FSFIX       = 28            ;   and its free map repaired
SHC_WAIT        = 30            ; .A = a task the shell started: it has the console (if the shell has), till it ends
SHC_LIBOPEN     = 32            ; .A.Y = a HyForth library's name (no .hyl): found (run.s) and opened.  OUT: .A = its fd
SHC_COUNT       = 34

SH_RUN_FD       = IO_MAX_FDS - 1    ; A program's file, for the task that runs it (SHC_RUN)
SH_ARGS_FD      = IO_MAX_FDS - 2    ;   and its arguments (a pipe: the loader reads them to HYX_ARGS)

; A Hydra executable (.hyx): this header, then the code, which is loaded at its load address in a task
; of its own, and called at its entry point (on ROM page 0: the $F8xx calls work).  The task ends when it
; returns.  (Not relocatable: a program is linked for its load address.)
HYX_MAGIC       = 0             ; "HYX1"
HYX_LOAD        = 4             ; Load address: HYX_RAM_LOW ... (the code's end <= HYX_RAM_END)
HYX_LENGTH      = 6             ; The code's length (after the header)
HYX_ENTRY       = 8             ; Entry point
HYX_FLAGS       = 10            ; Flags (0)
HYX_HEADER_SIZE = 16            ;   (12-15: reserved, 0)
HYX_RAM_LOW     = $0800         ; The task RAM a program can have
HYX_RAM_END     = $7C00         ;   (above it: the MMU's pages, and the task system page)
HYX_ARGS        = $0110         ; Its arguments, zero-terminated (.A.Y at the entry point: the rest of the
HYX_ARGS_SIZE   = 64            ;   line after its name), at the bottom of the stack page
