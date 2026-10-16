/*
** term.c - Dingus Quest Zero's screen, on the console as an ANSI terminal: the status line at the top (reverse
** video), the story under it scrolling in a region of its own (DECSTBM: rows 2 to the next to last), its words
** wrapped to the screen's width, and the line being typed on the bottom row, outside the region, so whatever the
** dungeon does while the player types scrolls past above it and the line stays whole.
**
** Everything goes out through a buffer of this file's, written to fd 1 at t_done (and when it's full): no stdio, no
** printf (sayf is this file's own, with %s %d %u %c).  The keys are this file's too (conio's kbhit and cgetc, and
** its screensize, without the stdio they take in): read raw from the window's console.
*/

#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "dq0.h"

unsigned char tw, th;                           /* The screen */
unsigned char t_ready;                          /* The character can act: the line typed's prompt "> " (else "  ") */
char inl[INMAX + 1];                            /* The line being typed, */
unsigned char inn;                              /*   its length */

static char ob[128];                            /* What's to go out, */
static unsigned char on;                        /*   how much */
static unsigned char col;                       /* The story's column: where its next word goes */
static unsigned char gap;                       /* A space is owed before the next word */
static unsigned char blanked;                   /* The story's last line is empty */
static unsigned char away;                      /* The cursor isn't at the end of the line typed */
static unsigned char shown;                     /* How much of the line typed is shown */
#define QMAX        256                         /* Lines entered and not yet taken (typed ahead): */
static char queue[QMAX];                        /*   each ends with a 0, */
static unsigned qlen;                           /*   so many bytes of them */
static char last[INMAX + 1];                    /* The last line entered (cursor up brings it back) */
static char sl[128], sn[128];                   /* The status line as it's shown, and as it's to be */

/* ---- Output */

static void flush (void)
{
    if (on) {
        write (1, ob, on);
        on = 0;
    }
}

static void __fastcall__ out (char c)
{
    if (on == sizeof ob) {
        flush ();
    }
    ob[on++] = c;
}

static void __fastcall__ outs (const char* s)
{
    while (*s) {
        out (*s++);
    }
}

static void __fastcall__ outn (unsigned n)
{
    char b[6];

    outs (utoa (n, b, 10));
}

static void csi (void)
{
    out (0x1B);
    out ('[');
}

/* The cursor to row r, column c (from 1) */
static void __fastcall__ go (unsigned char r, unsigned char c)
{
    csi ();
    outn (r);
    out (';');
    outn (c);
    out ('H');
}

/* The cursor hidden while it's away from the line typed (ESC [ ? 25 l: the console's, and its terminals'), and
** shown again as it's back (t_done, inl_show) */
static unsigned char hid;

static void hide (void)
{
    if (!hid) {
        csi ();
        outs ("?25l");
        hid = 1;
    }
}

static void unhide (void)
{
    if (hid) {
        csi ();
        outs ("?25h");
        hid = 0;
    }
}

/* The cursor to the story's next place (its last row, at col), if it isn't there */
static void to_story (void)
{
    if (away != 2) {
        hide ();
        go (th - 1, col + 1);
        away = 2;
    }
}

static void newline (void)
{
    to_story ();
    out ('\r');
    out ('\n');
    blanked = col == 0;
    col = 0;
    gap = 0;
}

/* ---- The keys: the window's console read raw (main.c's consctl: rawon), each key one code (hydra.h's CH_*), on
** an fd of its own, and looked for without waiting on another */

int __fastcall__ _hy_open (const char* name, unsigned char mode);     /* (The C library's) */
static int kfd = -1, nbfd = -1;
static int pend = -1;                           /* A key t_haskey saw */

static void keys (void)
{
    if (kfd < 0) {
        consctl ("rawon");
        kfd = _hy_open ("/dev/cons", HY_O_READ);
        nbfd = _hy_open ("/dev/cons", HY_O_READ | HY_O_NONBLOCK);
    }
}

unsigned char t_haskey (void)
{
    unsigned char c;

    keys ();
    if (pend < 0 && read (nbfd, &c, 1) == 1) {
        pend = c;
    }
    return pend >= 0;
}

unsigned char t_getkey (void)
{
    unsigned char c;

    keys ();
    if (pend >= 0) {
        c = pend;
        pend = -1;
    } else {
        while (read (kfd, &c, 1) != 1) {
            hy_yield ();
        }
    }
    return c == '\r' ? '\n' : c;
}

/* The window's size: its consctl's size line (C R), else $COLUMNS and $LINES, else 80 by 24; and whether the console
** shows its terminals both (the serial port's and the Vera X's screen: "terminal both") */
static unsigned char both, whole;               /* (Both; the screen alone made so by the game, put back at its end) */
static void size (void)
{
    static char b[100];
    char* p;
    int fd, n = 0;

    tw = 80;
    th = 24;
    if ((fd = _hy_open ("/dev/consctl", HY_O_READ)) >= 0) {
        n = read (fd, b, sizeof b - 1);
        close (fd);
    }
    b[n > 0 ? n : 0] = 0;
    for (p = b; p; p = strchr (p, '\n')) {
        if (*p == '\n') {
            ++p;
        }
        if (!strncmp (p, "terminal both", 13)) {
            both = 1;
        }
        if (!strncmp (p, "size ", 5)) {
            tw = atoi (p + 5);
            if ((p = strchr (p + 5, ' ')) != 0) {
                th = atoi (p + 1);
            }
        }
    }
    if (tw != 80 || th != 24) {
        return;                                 /* (The console's) */
    }
    if ((p = getenv ("COLUMNS")) != 0 && atoi (p) > 0) {
        tw = atoi (p);
    }
    if ((p = getenv ("LINES")) != 0 && atoi (p) > 0) {
        th = atoi (p);
    }
}

/* ---- The screen */

void t_init (void)
{
    static unsigned char once;

    size ();
    if (!once) {                                /* (With a Vera X the game takes its whole screen, the user's: the
                                                **   console's terminals both, as it starts, would make the window
                                                **   the serial port's 80 by 24; put back as the game ends) */
        once = 1;
        if (both) {
            consctl ("terminal screen");
            whole = 1;
            size ();
        }
    }
    if (tw < 40) {
        tw = 40;
    }
    if (th < 8) {
        th = 8;
    }
    csi ();
    outs ("2J");                                /* Cleared, */
    csi ();
    out ('2');                                  /*   the story's region rows 2 to th - 1 */
    out (';');
    outn (th - 1);
    out ('r');
    col = gap = 0;
    blanked = 1;
    away = 1;
    memset (sl, 0, sizeof sl);                  /* (The status line drawn whole) */
    status ();
    inl_show ();
    t_done ();
}

void t_end (void)
{
    csi ();
    out ('r');                                  /* The whole screen scrolls again */
    go (th, 1);
    csi ();
    out ('K');
    unhide ();
    flush ();
    if (whole) {
        consctl ("terminal both");              /* (The console's terminals as they were) */
        whole = 0;
    }
}

/* The status line: only what's changed since it was last drawn (the clock's last digit, mostly), the cursor hidden
** while it's away from the line typed (the user: the cursor flickered, the line drawn whole each game second); nothing
** at all, the cursor left where it is, if nothing has */

void __fastcall__ t_status (const char* left, const char* right)
{
    unsigned char n = strlen (left), m = strlen (right), k, a, b;

    memset (sn, ' ', tw);
    memcpy (sn + 1, left, n < tw - 2 ? n : tw - 2);
    if (m + n + 3 <= tw) {
        memcpy (sn + tw - 1 - m, right, m);
    }
    for (a = 0; a < tw && sn[a] == sl[a]; ++a) {
    }
    if (a == tw) {
        return;
    }
    for (b = tw - 1; sn[b] == sl[b]; --b) {
    }
    hide ();
    go (1, a + 1);
    csi ();
    outs ("7m");
    for (k = a; k <= b; ++k) {
        out (sl[k] = sn[k]);
    }
    csi ();
    outs ("0m");
    away = 1;
}

/* ---- The story */

void __fastcall__ say (const char* s)
{
    const char* w;
    unsigned char n;

    to_story ();
    while (*s) {
        if (*s == '\n') {
            newline ();
            ++s;
            continue;
        }
        if (*s == ' ') {
            ++s;
            gap = col != 0;
            continue;
        }
        for (w = s, n = 0; *s && *s != ' ' && *s != '\n'; ++s) {
            ++n;
        }
        if (col && col + gap + n >= tw) {       /* (The word doesn't fit: the next line) */
            newline ();
        }
        if (gap) {
            out (' ');
            ++col;
            gap = 0;
        }
        while (w < s) {
            if (col >= tw - 1) {                /* (A word longer than the line) */
                newline ();
            }
            out (*w++);
            ++col;
        }
        blanked = 0;
    }
}

/* A line of the story as it is (the map's): not wrapped, cut at the screen's width */
void __fastcall__ say_raw (const char* s)
{
    if (col) {
        newline ();
    }
    to_story ();
    while (*s && col < tw - 1) {
        out (*s++);
        ++col;
    }
    blanked = 0;
    newline ();
}

void __fastcall__ msg (const char* s)
{
    if (col) {
        newline ();
    }
    say (s);
}

void blank (void)
{
    if (col) {
        newline ();
    }
    if (!blanked) {
        newline ();
    }
}

/* The story's printf: %s, %d, %u, %c and %% */
void sayf (const char* f, ...)
{
    static char b[200];
    char n[8];
    va_list ap;
    unsigned char k = 0;
    const char* s;

    va_start (ap, f);
    for (; *f && k < sizeof b - 8; ++f) {
        if (*f != '%') {
            b[k++] = *f;
            continue;
        }
        switch (*++f) {
        case 's':
            s = va_arg (ap, const char*);
            break;
        case 'd':
            s = itoa (va_arg (ap, int), n, 10);
            break;
        case 'u':
            s = utoa (va_arg (ap, unsigned), n, 10);
            break;
        case 'c':
            n[0] = va_arg (ap, int);
            n[1] = 0;
            s = n;
            break;
        default:
            n[0] = *f;
            n[1] = 0;
            s = n;
            break;
        }
        while (*s && k < sizeof b - 1) {
            b[k++] = *s++;
        }
    }
    va_end (ap);
    b[k] = 0;
    say (b);
}

/* ---- The line being typed */

void __fastcall__ t_prompt (unsigned char ready)
{
    if (ready != t_ready) {
        t_ready = ready;
        away = 1;                               /* (Drawn again at t_done) */
    }
}

void inl_show (void)
{
    unsigned char from = inn > tw - 4 ? inn - (tw - 4) : 0;

    go (th, 1);                                 /* (Cleared first: with nothing typed, "> " is the last thing out,
                                                **   a prompt for a test to wait for) */
    csi ();
    out ('K');
    unhide ();
    outs (t_ready ? "> " : "  ");
    outs (inl + from);
    shown = inn - from;
    away = 0;
}

void t_done (void)
{
    if (away) {
        if (shown) {
            go (th, 3 + shown);
            unhide ();
        } else {
            go (th, 1);                         /* (Nothing typed: "> " again, the last thing out, a prompt) */
            csi ();
            out ('K');
            unhide ();
            outs (t_ready ? "> " : "  ");
        }
        away = 0;
    }
    flush ();
}

/* A line entered: into the story as the player typed it, then queued for t_line (no room: the line stays on the
** line typed, and Enter must be pressed again) */
static void enter (void)
{
    if (qlen + inn + 1 > QMAX) {
        return;
    }
    memcpy (queue + qlen, inl, inn + 1);
    qlen += inn + 1;
    if (inn) {
        strcpy (last, inl);
    }
    blank ();
    say ("> ");
    say (inl);
    inn = 0;
    inl[0] = 0;
    shown = 0;                                  /* (The line typed drawn again by t_done, once the command's done:
                                                **   its "> " a prompt only when the game's ready) */
    away = 1;
}

void __fastcall__ t_key (unsigned char k)
{
    if (k == '\n' || k == '\r') {
        enter ();
    } else if (k == 8 || k == 0x7F) {           /* Backspace, Delete */
        if (inn) {
            inl[--inn] = 0;
            if (!away && inn < shown) {
                outs ("\b \b");
                --shown;
            } else {
                inl_show ();
            }
        }
    } else if (k == 0x15) {                     /* Ctrl-U: the line cleared */
        inn = 0;
        inl[0] = 0;
        inl_show ();
    } else if (k == CH_CURS_UP) {               /* The last line again */
        strcpy (inl, last);
        inn = strlen (inl);
        inl_show ();
    } else if (k >= ' ' && k < 0x7F && inn < INMAX) {
        inl[inn++] = k;
        inl[inn] = 0;
        if (!away && shown < tw - 4) {
            out (k);
            ++shown;
        } else {
            inl_show ();
        }
    }
}

const char* t_next (void)
{
    return qlen ? queue : 0;
}

char* t_line (void)
{
    static char taken[INMAX + 1];
    unsigned n;

    if (!qlen) {
        return 0;
    }
    n = strlen (queue) + 1;                     /* (The first line, out of the queue) */
    memcpy (taken, queue, n);
    qlen -= n;
    memmove (queue, queue + n, qlen);
    return taken;
}

/* ---- Questions, while the world waits (the title, the character made) */

unsigned char __fastcall__ ask (const char* keys)
{
    const char* p;
    char k;

    t_done ();
    for (;;) {
        k = t_getkey ();
        if (k >= 'A' && k <= 'Z') {
            k += 'a' - 'A';
        }
        for (p = keys; *p; ++p) {
            if (*p == k) {
                return p - keys;
            }
        }
    }
}

void __fastcall__ ask_line (char* b, unsigned char max)
{
    char* l;

    inn = 0;
    inl[0] = 0;
    inl_show ();
    t_done ();
    while ((l = t_line ()) == 0) {
        t_key (t_getkey ());
        t_done ();
    }
    strncpy (b, l, max);
    b[max] = 0;
}
