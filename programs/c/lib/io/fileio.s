; ****************************************************************************
; fileio.s - the C library's file calls on the Hydra's IO layer (fds 0-2: stdin, stdout, stderr, as the
; shell gives them): the raw reads and writes (read.c and write.c add a terminal's line ends for the console),
; close, and the helpers open.c and lseek.c use.  A failed call sets _oserror (the OS's error) and errno
; (___mappederrno), and returns -1.

        .export     __hy_read, __hy_write, _close
        .export     __hy_open, __hy_create, __hy_seek, __hy_size, __hy_statrec
        .import     popax, ___mappederrno

        .include    "zeropage.inc"
        .include    "hydra.inc"

        .code

; int __fastcall__ _hy_read (int fd, void* buf, unsigned count): the bytes as they come
__hy_read:
        jsr         setbuf
        jsr         IO_READ
        bra         done

; int __fastcall__ _hy_write (int fd, const void* buf, unsigned count): the bytes as they are
__hy_write:
        jsr         setbuf
        jsr         IO_WRITE

done:                                                   ; The count, or the error
        bcs         error
        lda         ZP_IO_CNT
        ldx         ZP_IO_CNT + 1
        rts

error:
        jmp         ___mappederrno                      ; (C = 1: .A = the OS's error: -1)

; ZP_IO_CNT = .A.X, ZP_IO_BUF = the buffer, .A = the fd (both from the C stack)
setbuf:
        sta         ZP_IO_CNT
        stx         ZP_IO_CNT + 1
        jsr         popax
        sta         ZP_IO_BUF
        stx         ZP_IO_BUF + 1
        jmp         popax

; int __fastcall__ close (int fd)
_close:
        jsr         IO_CLOSE
        bcs         error
        lda         #0
        tax
        rts

; int __fastcall__ _hy_open (const char* name, unsigned char mode): the fd, or -1
__hy_open:
        sta         tmp1                                ; The mode
        jsr         popax                               ; The name
        pha
        txa
        tay
        pla
        ldx         tmp1
        jsr         IO_OPEN
        bcs         error
        ldx         #0
        rts

; int __fastcall__ _hy_create (const char* name, int mode, unsigned char bits): a new file (or directory: bits
; HFS_M_DIR), opened: the fd, or -1
__hy_create:
        sta         ZP_IO_BUF                           ; Its HFS_M_* bits
        jsr         popax
        sta         tmp1                                ; The mode
        jsr         popax                               ; The name
        pha
        txa
        tay
        pla
        ldx         tmp1
        jsr         IO_CREATE
        bcs         error
        ldx         #0
        rts

; int __fastcall__ _hy_seek (int fd, long offset): 0, or -1
__hy_seek:
        sta         ZP_IO_OFS
        stx         ZP_IO_OFS + 1
        lda         sreg
        sta         ZP_IO_OFS + 2
        lda         sreg + 1
        sta         ZP_IO_OFS + 3
        jsr         popax                               ; The fd
        jsr         IO_SEEK
        bcs         error
        lda         #0
        tax
        rts

; int __fastcall__ _hy_statrec (int fd, unsigned char* rec): its stat record (IO_STAT_SIZE bytes: IO_STAT) in rec.
; 0, or -1
__hy_statrec:
        sta         ZP_IO_BUF
        stx         ZP_IO_BUF + 1
        jsr         popax                               ; The fd
        jsr         IO_STAT
        bcs         error
        lda         #0
        tax
        rts

; long __fastcall__ _hy_size (int fd): its size (IO_STAT), or -1
__hy_size:
        pha
        lda         #<stat
        sta         ZP_IO_BUF
        lda         #>stat
        sta         ZP_IO_BUF + 1
        pla
        jsr         IO_STAT
        bcs         @error
        lda         stat + IO_ST_SIZE + 2
        sta         sreg
        lda         stat + IO_ST_SIZE + 3
        sta         sreg + 1
        lda         stat + IO_ST_SIZE
        ldx         stat + IO_ST_SIZE + 1
        rts

@error:
        jsr         ___mappederrno
        stx         sreg                                ; (-1: all four bytes)
        stx         sreg + 1
        rts

        .bss
stat:       .res    IO_STAT_SIZE
