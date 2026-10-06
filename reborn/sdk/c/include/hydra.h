/*
** hydra.h - the Hydra-16's own calls and constants for C programs (cc65 -t none, with hydra.lib and hydra.cfg: the
** C SDK, sdk/c/README.md).  The standard library works on the Hydra's files and devices, in the manner of Plan 9:
** stdio (fds 0-2 are stdin, stdout and stderr, as rc gives them), open, read, write, lseek, stat and fstat,
** directories (dirent.h: stat records, no text to take apart), the environment (getenv, putenv, setenv, unsetenv:
** rc's variables), system() (a command line for rc -c), exit statuses (exit and main's value; hy_exits for a
** message: rc's $status, and hy_wait's), clock and sleep, signal (SIGINT: the interrupt note, Ctrl-C), conio (the
** console: an ANSI terminal, its keys raw), and malloc.
**   A failed call sets errno (cc65's: _oserror has the kernel's own code, HY_E_*; strerror and _stroserror say
** them) and returns -1.  hydracalls.h (made from the specification) has every call, error code and constant, each
** with HY_ before its name; hy_call makes any call.
*/

#ifndef _HYDRA_H
#define _HYDRA_H

#include <hydracalls.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

/* ---- Any call: by its address (HY_NAME), its registers in and out.  0, or the error code (and _oserror, errno) */

struct hy_regs {
    unsigned char   a, x, y;            /* .A, .X, .Y */
    unsigned        r[4];               /* r0-r3 */
};

int __fastcall__ hy_call (unsigned call, struct hy_regs* regs);

/* ---- Files and directories */

#define S_IFDIR             0x80        /* st_mode: a directory (with cc65's S_IREAD and S_IWRITE) */
#define S_ISDIR(m)          ((m) & S_IFDIR)

int __fastcall__ fstat (int fd, struct stat* st);
int __fastcall__ hy_dirstat (DIR* dir, struct stat* st);    /* readdir's last entry, whole */
int __fastcall__ isatty (int fd);                           /* fd: a console (#c's cons)? */

/* ---- The environment (rc's variables: getenv and putenv are stdlib.h's) */

int __fastcall__ setenv (const char* name, const char* value, int overwrite);
int __fastcall__ unsetenv (const char* name);

/* ---- The clock, the tick, sleeping */

unsigned long hy_time (void);                               /* The clock: seconds since 2000-01-01 (time(): since
                                                            **   1970); /dev/time sets it */

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC      HY_TICK_HZ  /* clock(): the ticks since the program started */
#endif

unsigned hy_ticks (void);                                   /* The tick count (it wraps after about 5.5 minutes) */
int __fastcall__ hy_sleep_ticks (unsigned ticks);           /* Up to 32767 ticks; a note ends it sooner (-1) */
void hy_yield (void);                                       /* Let the other tasks run */

/* ---- Tasks and exit statuses (Plan 9's: a code, 0 for success, and a message) */

unsigned char hy_task (void);                               /* This task (1-15) */
int __fastcall__ hy_spawn (const char* path, char* const* argv, unsigned char flags);
                                                            /* A program (its path through the namespace: /bin/ls)
                                                            **   in a new task, argv its arguments (NULL-ended;
                                                            **   argv[0] isn't one: the name), flags HY_SPAWN_*:
                                                            **   its task (then hy_wait), or -1 */
int __fastcall__ hy_wait (int task, char* msg);             /* Wait for a task this one started to end (-1: any):
                                                            **   its exit code (0-255), its message into msg
                                                            **   (HY_EXIT_MSG_MAX + 1 bytes; NULL: not wanted) */
void __fastcall__ hy_exits (const char* msg);               /* End with a message (Plan 9's exits): NULL or "" is
                                                            **   success (0), anything else 1 and the message */
int __fastcall__ hy_note (int task, unsigned char note);    /* A note (HY_NOTE_*) to a task (HY_NOTE_GROUP |
                                                            **   a group: its tasks) */
unsigned char hy_parent (void);                             /* The task that started this one (0xFF: none) */

/* ---- Semaphores: every task's, by number; a mutex (HY_SEM_MUTEX) is given back only by the task that took it */

int __fastcall__ hy_sem_new (unsigned char count, unsigned char flags);  /* One: its number, or -1 */
int __fastcall__ hy_sem_acquire (unsigned char sem);        /* Take one, waiting till there's one (a note ends
                                                            **   the wait: -1, EINTR) */
int __fastcall__ hy_sem_try (unsigned char sem);            /* Take one if there's one (none: -1, EAGAIN) */
int __fastcall__ hy_sem_release (unsigned char sem);        /* Give one back, and wake its waiters */
int __fastcall__ hy_sem_free (unsigned char sem);           /* Free it (its waiters' waits end: -1) */

/* ---- The namespace, Plan 9's: a bind or mount with no flags replaces what's at old; HY_MBEFORE and HY_MAFTER
** add to old's union, before or after its members; HY_MCREATE: a file made in the union is made in it */

int __fastcall__ hy_bind (const char* new, const char* old, unsigned char flags);
int __fastcall__ hy_mount (char dev, const char* spec, const char* old, unsigned char flags);  /* '#dev' and spec */
int __fastcall__ hy_unmount (const char* new, const char* old);     /* old's member new (NULL: all of old's) */

/* ---- RAM banks: the task's own, 8K each, at HY_BANK_WINDOW ($8000-$9FFF) when selected (hy_bank) */

#define HY_BANK_WINDOW      ((unsigned char*) 0x8000)
#define hy_bank(b)          (*(volatile unsigned char*) 0 = (b))    /* (Its bank register, $00) */

unsigned char hy_banks (void);                              /* The banks it has (16 a RAM module) */
int __fastcall__ hy_banks_alloc (unsigned char n);          /* n of them, in a run: the first, or -1 */
int __fastcall__ hy_banks_free (unsigned char first, unsigned char n);

/* ---- Errors */

const char* __fastcall__ hy_errstr (unsigned char code);    /* An error code's text (ERRSTR) */

/* ---- conio's keys (cgetc): the console's raw codes for the terminal's cursor and function keys */

#define CH_CURS_UP          HY_KEY_UP
#define CH_CURS_DOWN        HY_KEY_DOWN
#define CH_CURS_LEFT        HY_KEY_LEFT
#define CH_CURS_RIGHT       HY_KEY_RIGHT
#define CH_HOME             HY_KEY_HOME
#define CH_END              HY_KEY_END
#define CH_INS              HY_KEY_INS
#define CH_DEL              HY_KEY_DEL
#define CH_PAGE_UP          HY_KEY_PGUP
#define CH_PAGE_DOWN        HY_KEY_PGDN
#define CH_F1               HY_KEY_F1                       /* (F1-F12: HY_KEY_F1 + 0-11) */
#define CH_F2               HY_KEY_F2
#define CH_F3               HY_KEY_F3
#define CH_F4               HY_KEY_F4
#define CH_F5               HY_KEY_F5
#define CH_F6               HY_KEY_F6
#define CH_F7               HY_KEY_F7
#define CH_F8               HY_KEY_F8
#define CH_F9               HY_KEY_F9
#define CH_F10              HY_KEY_F10
#define CH_ENTER            '\n'
#define CH_ESC              0x1B

/* ---- conio's colours (textcolor, bgcolor): the terminal's 8, and their bright 8 */

#define COLOR_BLACK         0
#define COLOR_RED           1
#define COLOR_GREEN         2
#define COLOR_YELLOW        3
#define COLOR_BLUE          4
#define COLOR_MAGENTA       5
#define COLOR_CYAN          6
#define COLOR_WHITE         7
#define COLOR_GRAY          8
#define COLOR_LIGHTRED      9
#define COLOR_LIGHTGREEN    10
#define COLOR_LIGHTYELLOW   11
#define COLOR_LIGHTBLUE     12
#define COLOR_LIGHTMAGENTA  13
#define COLOR_LIGHTCYAN     14
#define COLOR_BRIGHTWHITE   15

#endif
