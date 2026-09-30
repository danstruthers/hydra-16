/*
** hydra.h - the Hydra-16's own calls and constants for C programs (cc65; the library: programs/c/lib).
**
** The standard library works on the Hydra's files and devices, in the manner of Plan 9: stdio (fds 0-2 are
** stdin, stdout and stderr, as the shell gives them), the environment (getenv, putenv, setenv, unsetenv: each
** variable is a file, /env/NAME), system() (a command line, run as the shell runs one), exit statuses (exit and
** main's value; hy_exits for a message: the shell's $status, and hy_wait's), stat and fstat, directories
** (dirent.h), time and clock, sleep, conio (the console: an ANSI terminal), and malloc.
** A failed call sets errno (and _oserror, the Hydra's own error: os_rom/include/kernel.inc) and returns -1.
*/

#ifndef _HYDRA_H
#define _HYDRA_H

#include <time.h>

/* ---- Sizes */

#define HY_PATH_MAX         65          /* A path, with its 0 (getcwd's buffer, say: FILENAME_MAX is 17 here) */
#define HY_NAME_MAX         32          /* A file's name (one part of a path), with its 0 */
#define HY_ENV_MAX          255         /* An environment variable's value: its characters, at most */
#define HY_STATUS_MAX       31          /* An exit status's message, with its 0 (hy_wait's buffer) */
#define HY_STAT_SIZE        48          /* A stat record (the IO layer's) */

/* ---- The scheduler, and the time */

#define HY_TICKS_PER_SEC    200         /* The scheduler's tick */
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC      HY_TICKS_PER_SEC    /* clock(): ticks since the program started */
#endif

unsigned hy_ticks (void);                               /* The tick count (it wraps) */
void __fastcall__ hy_sleep_ticks (unsigned ticks);      /* Sleep (up to 32767 ticks): other tasks run */
void hy_yield (void);                                   /* Let the other tasks run */
unsigned long hy_clock (void);                          /* The clock: seconds since 2000-01-01 00:00:00 */

/* ---- Tasks and exit statuses (Plan 9's: a code, 0 for success, and a message) */

unsigned char hy_task (void);                           /* This task's number (1-15, as ps shows it) */
int __fastcall__ hy_spawn (const char* cmd);            /* A command line in a new task, not waited for: its
                                                        **   task (then hy_wait), or -1 */
int __fastcall__ hy_wait (int task, char* msg);         /* Wait for a task this one started to end: its exit
                                                        **   code (0-255), its message into msg (HY_STATUS_MAX;
                                                        **   NULL: not wanted) */
void __fastcall__ hy_exits (const char* msg);           /* End with a message (Plan 9's exits): NULL or "" is
                                                        **   success (0), anything else 1 and the message */
int __fastcall__ hy_kill (int task);                    /* End a task and the tasks it started (its status:
                                                        **   137, "killed") */

/* ---- Semaphores: 16 for the whole system, by number (1-16), so a task can hand one to the tasks it starts.
** A counting semaphore: acquire takes one, waiting (using no CPU) until there's one; release gives one back.  A
** mutex: a semaphore of 1 that only the task that took it can release.  When a task ends, the semaphores it
** made are freed and the mutexes it holds are released. */

int __fastcall__ hy_sem_new (unsigned char count);      /* A new semaphore of count (0-255) */
int hy_mutex_new (void);                                /* A new mutex */
int __fastcall__ hy_sem_acquire (unsigned char s);      /* Take one: 0 (it waits until there's one) */
int __fastcall__ hy_sem_try (unsigned char s);          /* Take one if there is one: 1; 0 if not (at once) */
int __fastcall__ hy_sem_release (unsigned char s);      /* Give one back: 0 */
int __fastcall__ hy_sem_free (unsigned char s);         /* Free it (the tasks waiting for it get -1): 0 */

/* ---- Files: what POSIX has and cc65's headers don't, for this target */

struct stat;
struct DIR;
int __fastcall__ fstat (int fd, struct stat* st);       /* stat, for an open file */
int __fastcall__ hy_dirstat (struct DIR* dir, struct stat* st);    /* The entry readdir gave last, as stat */
int __fastcall__ isatty (int fd);                       /* fd is the console (/dev/cons)? */
int __fastcall__ setenv (const char* name, const char* value, int overwrite);
int __fastcall__ unsetenv (const char* name);

#define S_IFDIR             0x04        /* stat's st_mode: a directory (S_IREAD, S_IWRITE: sys/stat.h) */
#define S_ISDIR(m)          (((m) & S_IFDIR) != 0)

/* ---- conio: colours (ANSI's: textcolor, bgcolor) */

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

/* ---- conio: the keys cgetc gives (the terminal's sequences for its cursor and function keys, decoded) */

#define CH_ENTER            13
#define CH_ESC              27
#define CH_DEL              127         /* (The Backspace key, on most terminals) */
#define CH_CURS_UP          0x80
#define CH_CURS_DOWN        0x81
#define CH_CURS_LEFT        0x82
#define CH_CURS_RIGHT       0x83
#define CH_HOME             0x84
#define CH_END              0x85
#define CH_PAGE_UP          0x86
#define CH_PAGE_DOWN        0x87
#define CH_INS              0x88
#define CH_DELETE           0x89        /* (The Delete key: ESC [ 3 ~) */
#define CH_F1               0x8A
#define CH_F2               0x8B
#define CH_F3               0x8C
#define CH_F4               0x8D

/* ---- conio: lines (chline, cvline) */

#define CH_HLINE            '-'
#define CH_VLINE            '|'

#endif
