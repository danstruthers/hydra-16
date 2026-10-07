; ****************************************************************************
; _cwd.s - the current directory for getcwd and chdir: __cwd, filled in at the start (a constructor) and by
; chdir.  (It replaces cc65's _cwd.o, whose buffer, for targets it doesn't know, holds 16 characters; the
; Hydra's paths can be 63.)

        .export     __cwd, __cwd_buf_size, initcwd, __syschdir
        .constructor initcwd

        .include    "hydra.inc"

__cwd_buf_size = 64 + 1

        .code

; void initcwd (void): __cwd = the task's current directory (IO_GETCWD)
initcwd:
        lda         #<__cwd
        sta         ZP_IO_BUF
        lda         #>__cwd
        sta         ZP_IO_BUF + 1
        jsr         IO_GETCWD
        bcc         @done
        lda         #0                                  ; (Unknown)
        sta         __cwd
@done:
        rts

; unsigned char __fastcall__ _syschdir (const char* name): the current directory (IO_CHDIR), and __cwd
__syschdir:
        pha
        txa
        tay
        pla
        jsr         IO_CHDIR
        bcs         @done
        jsr         initcwd
        lda         #0
@done:
        rts

        .segment    "INIT"
__cwd:      .res    __cwd_buf_size
