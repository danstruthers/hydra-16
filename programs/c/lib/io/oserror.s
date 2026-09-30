; ****************************************************************************
; oserror.s - int __fastcall__ __osmaperrno (unsigned char oserror): the Hydra's errors (kernel.inc) as
; errno's (EUNKNOWN for the rest).  _oserror keeps the Hydra's own.

        .export     ___osmaperrno

        .include    "errno.inc"
        .include    "hydra.inc"

        .code

___osmaperrno:
        ldx         #ERRTAB_SIZE - 2

@look:
        cmp         errtab,x
        beq         @found
        dex
        dex
        bpl         @look
        lda         #<EUNKNOWN
        ldx         #>EUNKNOWN
        rts

@found:
        lda         errtab + 1,x
        ldx         #0
        rts

        .rodata

errtab:
        .byte       ERR_OUT_OF_MEMORY,  ENOMEM
        .byte       ERR_MEM_NOT_VALID,  EINVAL
        .byte       ERR_MEM_BAD_ARG,    EINVAL
        .byte       ERR_SEM_BAD,        EINVAL
        .byte       ERR_SEM_NONE,       EAGAIN
        .byte       ERR_SEM_BUSY,       EAGAIN
        .byte       ERR_SEM_NOT_HELD,   EACCES
        .byte       ERR_SEM_FULL,       ERANGE
        .byte       ERR_IO_NOT_FOUND,   ENOENT
        .byte       ERR_IO_BAD_FD,      EBADF
        .byte       ERR_IO_MODE,        EACCES
        .byte       ERR_IO_WOULD_BLOCK, EAGAIN
        .byte       ERR_IO_NO_FDS,      EMFILE
        .byte       ERR_IO_NAME,        EINVAL
        .byte       ERR_IO_BAD_REQ,     ENOSYS
        .byte       ERR_IO_DEVICE,      ENODEV
        .byte       ERR_IO_BROKEN,      EIO
        .byte       ERR_IO_NOT_READY,   ENODEV
        .byte       ERR_IO_MEDIA,       EIO
        .byte       ERR_IO_NOT_FS,      ENODEV
        .byte       ERR_IO_FULL,        ENOSPC
        .byte       ERR_IO_EXISTS,      EEXIST
        .byte       ERR_IO_NOT_EMPTY,   EACCES
        .byte       ERR_IO_BUSY,        EBUSY
        .byte       ERR_IO_NOT_DIR,     EINVAL
        .byte       ERR_IO_IS_DIR,      EACCES
        .byte       ERR_IO_NOT_EXEC,    ENOEXEC
ERRTAB_SIZE = * - errtab
.assert     ERRTAB_SIZE < 128, error, "errtab: .X counts down with bpl"
