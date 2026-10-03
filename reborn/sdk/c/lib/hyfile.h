/*
** hyfile.h - the C library's stdio buffers (hyfile.c), and cc65's FILE (its _file.h, which cc65's include folder
** doesn't have).  Each fd has a buffer, made at a FILE's first read or write on it: FB_SIZE bytes (malloc), or one
** byte (stderr's: unbuffered; or no memory).  A console's is line buffered: what's written goes out at each LF;
** and stdout's waiting output goes out before any read (a prompt before its answer).  What's waiting goes out at
** fflush, fclose, a seek and the program's end (atexit).  cc65's stdio modules that read or write are the
** library's own for them: fgetc.s, fread.c, fwrite.c (fputc, fwrite, fputs, puts), fseek.c (fseek, ftell),
** fclose.c (fclose, freopen) and fmisc.c (fflush, clearerr, feof, ferror, fileno).
*/

#ifndef _HYFILE_H
#define _HYFILE_H

#include <stdio.h>
#include <hydra.h>

/* cc65's FILE: its fd, flags, and a byte ungetc put back */
struct _FILE {
    char            f_fd;
    char            f_flags;
    unsigned char   f_pushback;
};
extern FILE _filetab[FOPEN_MAX];

#define _FCLOSED        0x00
#define _FOPEN          0x01
#define _FEOF           0x02
#define _FERROR         0x04
#define _FPUSHBACK      0x08

FILE* __fastcall__ _fopen (const char* name, const char* mode, FILE* f);

/* An fd's buffer (8 bytes: fgetc.s finds the fd's at fd * 8, its fields at these offsets) */
struct hy_fbuf {
    unsigned char*  buf;                /* The buffer (0: not made yet) ... */
    unsigned char   n;                  /*   reading: the bytes in it; writing: the bytes waiting ... */
    unsigned char   at;                 /*   reading: the next one's place ... */
    unsigned char   mode;               /*   FB_READ, FB_WRITE or 0 (neither yet) ... */
    unsigned char   flags;              /*   FB_LINE, FB_ONE ... */
    unsigned char   one;                /*   and the buffer of one byte (FB_ONE) */
    unsigned char   pad;
};
extern struct hy_fbuf _hy_fbuf[HY_FD_MAX];

#define FB_SIZE         255             /* A buffer's bytes (of BUFSIZ: n and at are bytes) */
#define FB_READ         1
#define FB_WRITE        2
#define FB_LINE         0x01            /* A console's: line buffered */
#define FB_ONE          0x02            /* A buffer of one byte (unbuffered) */
#define FBUF(f)         (&_hy_fbuf[(unsigned char) (f)->f_fd])
#define FBSIZE(b)       ((b)->flags & FB_ONE ? 1 : FB_SIZE)

int __fastcall__ _hy_read (FILE* f, void* p, unsigned n);  /* A read for f (stdout's waiting output out first,
                                                            **   if it's a console's): the bytes; 0 (EOF:
                                                            **   _FEOF set); -1 (_FERROR set) */
int __fastcall__ _hy_fill (FILE* f);                        /* f's buffer filled: as _hy_read */
int __fastcall__ _hy_towrite (FILE* f);                     /* Ready to write (the buffer made, read-ahead given
                                                            **   back): 0, or -1 */
int __fastcall__ _hy_flush (FILE* f);                       /* What's waiting written (or read-ahead given back,
                                                            **   if it can be): 0, or -1 (_FERROR set) */
void _hy_flushall (void);                                   /* Every FILE's waiting bytes written */

#endif
