; ****************************************************************************
; fileio.s - the C library's file calls on the Hydra's (fds 0-2: stdin, stdout and stderr, as rc gives them):
; read, write, close and lseek, and the helpers open.c, stat.c and dirent.c use.  A failed call sets _oserror (the
; error code: hydra.h's HY_E_*) and errno (___mappederrno: oserror.s's map), and returns -1.  The console turns
; LF into CR LF itself, and its lines end with LF, so the bytes go as they are.

            .export     _read, _write, _close, _lseek
            .export     __hy_open, __hy_create, __hy_fstat, __hy_stat
            .import     popax, popeax, ___mappederrno

            .include    "zeropage.inc"
            .include    "hydra.inc"

            .code

; int __fastcall__ read (int fd, void* buf, unsigned count)
_read:
            jsr         args
            jsr         READ
            bcs         error
            rts                                             ; (.A/.X: the count)

; int __fastcall__ write (int fd, const void* buf, unsigned count)
_write:
            jsr         args
            jsr         WRITE
            bcs         error
            rts

; r1 = the count (.A/.X), r0 = the buffer, .A = the fd (both from the C stack)
args:
            sta         r1
            stx         r1 + 1
            jsr         popax
            sta         r0
            stx         r0 + 1
            jmp         popax

error:
            jmp         ___mappederrno                      ; (C = 1: .A = the error: -1)

; int __fastcall__ close (int fd)
_close:
            jsr         CLOSE
            bcs         error
zero:
            lda         #0
            tax
            rts

; off_t __fastcall__ lseek (int fd, off_t offset, int whence): the new offset, or -1.  cc65's whence (SEEK_CUR 0,
; SEEK_END 1, SEEK_SET 2) as SEEK's from (1, 2, 0)
_lseek:
            cmp         #3
            bcs         @inval
            tax
            lda         whence,X
            sta         tmp1
            jsr         popeax                              ; The offset: r0, r1
            sta         r0
            stx         r0 + 1
            lda         sreg
            sta         r1
            lda         sreg + 1
            sta         r1 + 1
            jsr         popax                               ; The fd
            ldx         tmp1
            jsr         SEEK
            bcs         @error
            lda         r1
            sta         sreg
            lda         r1 + 1
            sta         sreg + 1
            lda         r0
            ldx         r0 + 1
            rts

@inval:
            jsr         popeax                              ; (Its arguments, off the C stack)
            jsr         popax
            lda         #E_INVAL
@error:
            jsr         ___mappederrno
            stx         sreg                                ; (-1: all four bytes)
            stx         sreg + 1
            rts

; 0 (C = 0), or -1 (C = 1: .A = the error)
result:
            bcs         error
            jmp         zero

; int __fastcall__ _hy_open (const char* name, unsigned char mode): the fd, or -1
__hy_open:
            sta         tmp1                                ; The mode (O_READ, O_WRITE, O_RDWR; O_TRUNC)
            jsr         popax                               ; The name
            sta         r0
            stx         r0 + 1
            lda         tmp1
            jsr         OPEN
            bcs         error
            ldx         #0
            rts

; int __fastcall__ _hy_create (const char* name, int mode, unsigned char bits): a new file (or directory: bits
; DM_DIR), opened: the fd, or -1
__hy_create:
            sta         tmp2                                ; Its mode's high byte (DM_*)
            jsr         popax
            sta         tmp1                                ; The mode
            jsr         popax                               ; The name
            sta         r0
            stx         r0 + 1
            lda         tmp1
            ldx         tmp2
            jsr         CREATE
            bcs         error
            ldx         #0
            rts

; int __fastcall__ _hy_fstat (int fd, unsigned char* rec): fd's stat record (SR_SIZE bytes) in rec.  0, or -1
__hy_fstat:
            sta         r0
            stx         r0 + 1
            jsr         popax
            jsr         FSTAT
            jmp         result

; int __fastcall__ _hy_stat (const char* name, unsigned char* rec): the stat record of the file named in rec.  0, or
; -1
__hy_stat:
            sta         r1
            stx         r1 + 1
            jsr         popax
            sta         r0
            stx         r0 + 1
            jsr         STAT
            jmp         result

            .rodata
whence:     .byte       1, 2, 0                             ; (SEEK_CUR, SEEK_END, SEEK_SET)
