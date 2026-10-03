; ****************************************************************************
; _cwd.s - the current directory for getcwd and chdir: __cwd, filled in at the start (a constructor) and by chdir.
; (It takes the place of cc65's _cwd.o, whose buffer, for a target it doesn't know, holds 16 characters: the
; Hydra's paths can be 63.)

            .export     __cwd, __cwd_buf_size, initcwd, __syschdir
            .constructor initcwd

            .include    "hydra.inc"

__cwd_buf_size  = PATH_MAX + 1

            .code

; void initcwd (void): __cwd = the task's current directory (GETCWD)
initcwd:
            lda         #<__cwd
            sta         r0
            lda         #>__cwd
            sta         r0 + 1
            jmp         GETCWD

; unsigned char __fastcall__ _syschdir (const char* name): the current directory (CHDIR), and __cwd.  0, or the
; error code
__syschdir:
            sta         r0
            stx         r0 + 1
            jsr         CHDIR
            bcs         @done
            jsr         initcwd
            lda         #0
@done:
            rts

            .segment    "INIT"
__cwd:      .res        __cwd_buf_size
