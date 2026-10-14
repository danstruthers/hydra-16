/*
** term.c - Dingus Quest Zero's screen, on the console as an ANSI terminal: the status line at the top (reverse
** video), the story under it scrolling in a region of its own (DECSTBM: rows 2 to the next to last), its words
** wrapped to the screen's width, and the line being typed on the bottom row, outside the region, so whatever the
** dungeon does while the player types scrolls past above it and the line stays whole.
**
** Everything goes out through a buffer of this file's, written to fd 1 at t_done (and when it's full): no stdio, no
** printf (sayf is this file's own, with %s %d %u %c).  The keys come from conio (kbhit, cgetc: raw).
*/

#include <conio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "dq0.h"

unsigned char tw, th;                           /* The screen */
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

/* The cursor to the story's next place (its last row, at col), if it isn't there */
static void to_story (void)
{
    if (away != 2) {
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

/* ---- The screen */

void t_init (void)
{
    screensize (&tw, &th);
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
    flush ();
}

void __fastcall__ t_status (const char* left, const char* right)
{
    unsigned char n = strlen (left), m = strlen (right), k;

    go (1, 1);
    csi ();
    outs ("7m");
    out (' ');
    outs (left);
    for (k = n + 1; k + m + 1 < tw; ++k) {
        out (' ');
    }
    outs (right);
    out (' ');
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

void inl_show (void)
{
    unsigned char from = inn > tw - 4 ? inn - (tw - 4) : 0;

    go (th, 1);                                 /* (Cleared first: with nothing typed, "> " is the last thing out,
    csi ();                                     /*   a prompt for a test to wait for) */
    out ('K');
    outs ("> ");
    outs (inl + from);
    shown = inn - from;
    away = 0;
}

void t_done (void)
{
    if (away) {
        if (shown) {
            go (th, 3 + shown);
        } else {
            go (th, 1);                         /* (Nothing typed: "> " again, the last thing out, a prompt) */
            csi ();
            out ('K');
            outs ("> ");
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
    away = 1;                                   /*   its "> " a prompt only when the game's ready) */
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
        k = cgetc ();
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
        t_key (cgetc ());
        t_done ();
    }
    strncpy (b, l, max);
    b[max] = 0;
}
