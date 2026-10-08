/*
** edit.c - edit [file ...]: the screen editor (phase 9), nano's way.  What's typed goes in at the cursor; the Ctrl
** keys and the Meta keys (Esc then a key, or Alt with it: M-) are commands, the two lines at the bottom name the
** commonest, and ^G shows them all.  Up to 6 files open at once, each in a buffer of its own (^R opens another, M-,
** and M-. go between them, ^X closes one: the last, and edit ends), their text in the task's RAM banks (edit.h).
**   ^O save, asking the name (^S: without)    ^W find (M-W again, M-Q backwards; letters either case)
**   M-R replace (each asked: yes, no, all)    M-G go to a line ($: the last)
**   ^K cut the line (^K again: the next joins it in the cut buffer), or the block from the mark (M-A); M-6 copy
**   them; ^U paste (in any file)              M-U undo, M-E redo (typing a line, a paste, a cut: one each)
**   the arrows (^B ^F ^P ^N), Home and End (^A ^E), PgUp and PgDn (^Y ^V), M-\ and M-/ (the text's start, end)
**   Del (^D) deletes, Backspace rubs out, Tab is a tab (stops every 8), M-I auto-indent, M-X the help lines,
**   ^L the screen drawn again (as it is when the window's size changes)
** A file whose lines end CR LF is read with LF and written with CR LF again.  Ctrl-C does nothing (the console's
** interrupt: ignored); ^C, ^\ and ^] never reach a program, so nano's keys there are Meta keys here.
*/

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <conio.h>
#include <errno.h>
#include <unistd.h>
#include "edit.h"

#define CTRL(c)     ((c) & 0x1F)
#define ESC         27

#define SLOT        (sizeof (struct doc) + BSTATE)  /* A file's state put away: D, then blocks.s's */

unsigned char ndocs, cur;                       /* The files open, and the one shown (D) */
static unsigned char sbank;                     /* The files' states, put away: a slot each */
static unsigned char quit, cutting, wantok, autoindent, full;
static char pat[PAT_MAX + 1], rep[PAT_MAX + 1];
static char line[HY_PATH_MAX + 1];
static char nb[12];

static const char* why (void)
{
    return _stroserror (_oserror);
}

static char* num (unsigned long n)
{
    return ultoa (n, nb, 10);
}

/* The file shown's state put away in slot i, or taken from it */
static void putaway (unsigned char i)
{
    setbank (sbank);
    memcpy (BANK + i * SLOT, &D, sizeof D);
    memcpy (BANK + i * SLOT + sizeof D, bstate, BSTATE);
}

static void takeout (unsigned char i)
{
    setbank (sbank);
    memcpy (&D, BANK + i * SLOT, sizeof D);
    memcpy (bstate, BANK + i * SLOT + sizeof D, BSTATE);
}

/* The file shown made file i */
static void show (unsigned char i)
{
    putaway (cur);
    takeout (cur = i);
    wantok = 0;
    s_all ();
}

/* The file's lines: its LFs, and one more if its last byte isn't one */
static unsigned lines (void)
{
    lpos n = t_len ();

    if (!n) {
        return 0;
    }
    it_set (n - 1);
    return t_lines () - (it_next () == LF);
}

/* The message: the file's lines ("1 line", "N lines"), then more */
static void linesmsg (const char* more)
{
    static char m[40];
    unsigned n = lines ();

    strcpy (m, num (n));
    strcat (m, n == 1 ? " line" : " lines");
    strcat (m, more);
    s_msg (m);
}

/* A new buffer, for file name (0: none), shown.  0; or 1: it couldn't be (said) */
static unsigned char open_doc (const char* name)
{
    int r;

    if (ndocs == DOC_MAX) {
        s_msg ("6 files are open already");
        return 1;
    }
    if (ndocs) {
        putaway (cur);
    }
    if (t_new ()) {
        if (ndocs) {
            takeout (cur);
        }
        s_msg ("no room: no RAM bank");
        return 1;
    }
    u_new ();
    if (name) {
        strncpy (D.name, name, HY_PATH_MAX);
        r = t_load (name);
        if (r < 0 && _oserror == HY_E_NOENT) {
            s_msg2 (name, ": a new file");
        } else if (r) {
            s_msg2 (name, r < 0 ? ": can't be read" : ": too big (no room)");
            t_free ();
            if (ndocs) {
                takeout (cur);
            }
            return 1;
        } else {
            linesmsg (D.dos ? " (CR LF)" : "");
        }
    }
    cur = ndocs++;
    wantok = 0;
    s_all ();
    return 0;
}

/* The file shown closed: the next shown, or none (edit ends) */
static void close_doc (void)
{
    t_free ();
    setbank (sbank);                                    /* (The slots after it, down one) */
    memmove (BANK + cur * SLOT, BANK + (cur + 1) * SLOT, (ndocs - cur - 1) * SLOT);
    if (!--ndocs) {
        quit = 1;
        return;
    }
    if (cur >= ndocs) {
        cur = ndocs - 1;
    }
    takeout (cur);
    wantok = 0;
    s_all ();
}

/* ---- Moving */

static void want (void)
{
    if (!wantok) {
        D.want = colof ();
        wantok = 1;
    }
}

static void up (unsigned n)
{
    want ();
    while (n--) {
        t_bol ();
        if (t_prev () < 0) {
            break;
        }
    }
    tocol (D.want);
}

static void down (unsigned n)
{
    want ();
    while (n--) {
        t_eol ();
        if (t_next () < 0) {
            break;
        }
    }
    tocol (D.want);
}

/* A page up or down: the cursor and the screen's top both */
static void page (signed char dir)
{
    unsigned k = TH > 2 ? TH - 2 : 1, l;

    if (dir > 0) {
        down (k);
        l = D.topline + k;
    } else {
        up (k);
        l = D.topline > k ? D.topline - k : 0;
    }
    if (l > cline) {
        l = cline;
    }
    if (l + TH <= cline) {
        l = cline - TH + 1;
    }
    s_settop (l);
}

/* ---- Changing */

static void typed (unsigned char c)
{
    if (c == LF) {
        s_dirty ();
    } else {
        s_dirtyrow ();
    }
    if (t_ins (&c, 1)) {
        s_msg ("no room");
    }
    wantok = 0;
}

/* Enter: a new line (with auto-indent, the blanks the line began with, as far as the cursor) */
static void enter (void)
{
    unsigned char n = 0;
    unsigned k = 0;
    int c;

    if (autoindent) {
        it_set (cpos);
        while ((c = it_prev ()) >= 0 && c != LF) {
            ++k;
        }
        if (c == LF) {
            it_next ();
        }
        while (k-- && n < PAT_MAX && ((c = it_next ()) == ' ' || c == '\t')) {
            line[n++] = c;
        }
    }
    typed (LF);
    if (n) {
        D.ugroup = U_TYPE;
        t_ins ((unsigned char*) line, n);
    }
}

static void rubout (void)
{
    int c;

    if ((c = t_prev ()) < 0) {
        return;
    }
    if (c == LF) {
        s_dirty ();
    } else {
        s_dirtyrow ();
    }
    t_del (1, U_RUB, 0);
    wantok = 0;
}

static void del (void)
{
    int c = t_get ();

    if (c < 0) {
        return;
    }
    if (c == LF) {
        s_dirty ();
    } else {
        s_dirtyrow ();
    }
    t_del (1, U_DEL, 0);
}

static void mark (void)
{
    D.marked = !D.marked;
    D.mark = cpos;
    s_dirtyall ();
    s_msg (D.marked ? "mark set" : "mark unset");
}

/* What ^K and M-6 take: the block from the mark (unmarked), or the cursor's line (the cursor past it) */
static void region (lpos* a, lpos* b)
{
    if (D.marked) {
        *a = D.mark < cpos ? D.mark : cpos;
        *b = D.mark < cpos ? cpos : D.mark;
        D.marked = 0;
        s_dirtyall ();
    } else {
        t_bol ();
        *a = cpos;
        t_eol ();
        t_next ();
        *b = cpos;
    }
}

static void tocut (const unsigned char* s, unsigned n)
{
    if (c_add (s, n)) {
        full = 1;
    }
}

static void cut (void)
{
    lpos a, b;

    if (!cutting) {
        c_clear ();
    }
    region (&a, &b);
    t_goto (a);
    s_dirty ();
    full = 0;
    t_del (b - a, U_DEL, tocut);
    if (full) {
        s_msg ("the cut buffer's full: not all of it's there");
    }
    wantok = 0;
}

static void copy (void)
{
    lpos a, b;
    unsigned k, i;

    if (!cutting) {
        c_clear ();
    }
    region (&a, &b);
    it_set (a);
    full = 0;
    while (a < b) {
        k = b - a > CHUNK ? CHUNK : (unsigned) (b - a);
        for (i = 0; i < k; ++i) {
            bounce[i] = it_next ();
        }
        tocut (bounce, k);
        a += k;
    }
    s_msg (full ? "the cut buffer's full: not all of it's there" : "copied");
    wantok = 0;
}

static void paste (void)
{
    if (!c_len ()) {
        s_msg ("nothing's been cut");
        return;
    }
    s_dirty ();
    D.ugroup = U_PASTE0;
    if (c_paste ()) {
        s_msg ("no room");
    }
    wantok = 0;
}

/* ---- Finding */

/* A pattern asked for (what: Find, Replace, With), the last (old) shown in brackets: an empty line is it again.
** 1: it's in old; 0: cancelled (said) */
static unsigned char askpat (const char* what, char* old)
{
    static char q[40];

    strcpy (q, what);
    if (old[0]) {
        strcat (q, " [");
        strncat (q, old, 24);
        strcat (q, "]");
    }
    strcat (q, ":");
    line[0] = 0;
    if (!s_prompt (q, line, PAT_MAX)) {
        s_msg ("cancelled");
        return 0;
    }
    if (line[0]) {
        strcpy (old, line);
    }
    return 1;
}

/* pat found from the cursor on (dir 1) or back (-1), round from the other end if need be.  (ask: it's asked for) */
static void find (signed char dir, unsigned char ask)
{
    lpos p = NOWHERE;
    unsigned char n;

    if (ask && !askpat ("Find", pat)) {
        return;
    }
    if (!(n = strlen (pat))) {
        s_msg ("nothing to find (^W)");
        return;
    }
    if (dir > 0) {
        if ((p = t_find ((unsigned char*) pat, n, cpos + 1, 1)) == NOWHERE &&
            (p = t_find ((unsigned char*) pat, n, 0, 1)) != NOWHERE) {
            s_msg ("(round from the top)");
        }
    } else {
        if (cpos) {
            p = t_find ((unsigned char*) pat, n, cpos - 1, -1);
        }
        if (p == NOWHERE && (p = t_find ((unsigned char*) pat, n, t_len (), -1)) != NOWHERE) {
            s_msg ("(round from the bottom)");
        }
    }
    if (p == NOWHERE) {
        s_msg2 (pat, ": not found");
        return;
    }
    t_goto (p);
    wantok = 0;
}

static void replace (void)
{
    lpos p, from = cpos;
    unsigned count = 0;
    unsigned char n, rn, all = 0;
    int a;

    if (!askpat ("Replace", pat) || !pat[0] || !askpat ("With", rep)) {
        return;
    }
    n = strlen (pat);
    rn = strlen (rep);
    while ((p = t_find ((unsigned char*) pat, n, from, 1)) != NOWHERE) {
        t_goto (p);
        if (!all) {
            D.marked = 1;                               /* (It, shown reversed) */
            D.mark = p + n;
            s_render ();
            a = s_ask ("Replace this? (y)es (n)o (a)ll", "yna");
            D.marked = 0;
            s_dirtyall ();
            if (a < 0) {
                break;
            }
            if (a == 1) {
                from = p + 1;
                continue;
            }
            all = a == 2;
        }
        t_del (n, U_DEL, 0);
        if (t_ins ((unsigned char*) rep, rn)) {
            s_msg ("no room");
            return;
        }
        from = cpos;
        ++count;
    }
    s_dirtyall ();
    s_msg2 (num (count), " replaced");
    wantok = 0;
}

static void gotoline (void)
{
    line[0] = 0;
    if (!s_prompt ("Line:", line, 6) || !line[0]) {
        return;
    }
    t_gotoline (line[0] == '$' ? 0xFFFF : atoi (line) ? atoi (line) - 1 : 0);
    wantok = 0;
}

/* ---- Files */

/* The file shown written (ask: its name asked for; with none, always).  0; or 1: it wasn't (said) */
static unsigned char save (unsigned char ask)
{
    strcpy (line, D.name);
    if (ask || !line[0]) {
        if (!s_prompt ("Write to:", line, HY_PATH_MAX) || !line[0]) {
            s_msg ("not written");
            return 1;
        }
    }
    if (t_save (line)) {
        s_msg2 ("not written: ", why ());
        return 1;
    }
    strcpy (D.name, line);
    linesmsg (" written");
    return 0;
}

static void openfile (void)
{
    unsigned char i;

    line[0] = 0;
    if (!s_prompt ("Open:", line, HY_PATH_MAX) || !line[0]) {
        return;
    }
    for (i = 0; i < ndocs; ++i) {
        setbank (sbank);
        if (!strcmp (i == cur ? D.name : (char*) BANK + i * SLOT + offsetof (struct doc, name), line)) {
            if (i != cur) {
                show (i);
            }
            s_msg ("(it's open)");
            return;
        }
    }
    open_doc (line);
}

static void leave (void)
{
    int a;

    if (D.changed) {
        if ((a = s_ask ("Save the changes? (y)es (n)o", "yn")) < 0) {
            s_msg ("cancelled");
            return;
        }
        if (a == 0 && save (0)) {
            return;
        }
    }
    close_doc ();
}

/* ---- Keys */

/* A Meta key's command (k: the key after Esc) */
static void meta (unsigned char k)
{
    if (k >= 'A' && k <= 'Z') {
        k |= 0x20;
    }
    switch (k) {
    case 'u':
        if (!u_undo ()) {
            s_msg ("nothing to undo");
        }
        s_dirtyall ();
        wantok = 0;
        break;
    case 'e':
        if (!u_redo ()) {
            s_msg ("nothing to redo");
        }
        s_dirtyall ();
        wantok = 0;
        break;
    case 'a':
        mark ();
        break;
    case '6':
    case '^':
        copy ();
        cutting = 2;
        break;
    case 'w':
        find (1, 0);
        break;
    case 'q':
        find (-1, 0);
        break;
    case 'r':
        replace ();
        break;
    case 'g':
        gotoline ();
        break;
    case '\\':
        t_goto (0);
        wantok = 0;
        break;
    case '/':
        t_goto (t_len ());
        wantok = 0;
        break;
    case ',':
    case '<':
        if (ndocs > 1) {
            show (cur ? cur - 1 : ndocs - 1);
        }
        break;
    case '.':
    case '>':
        if (ndocs > 1) {
            show (cur + 1 < ndocs ? cur + 1 : 0);
        }
        break;
    case 'x':
        helpshown = !helpshown;
        s_all ();
        break;
    case 'i':
        autoindent = !autoindent;
        s_msg (autoindent ? "auto-indent on" : "auto-indent off");
        break;
    case ESC:
        break;
    default:
        s_msg ("not a key edit knows (^G: the keys)");
    }
}

/* A key's command.  1: it goes on typing, deleting or rubbing out (the undo log's record grows) */
static unsigned char command (unsigned char k)
{
    switch (k) {
    case ESC:
        meta (cgetc ());
        return 0;
    case '\n':
        enter ();
        return 1;
    case 8:
    case 127:
        rubout ();
        return 1;
    case CTRL ('D'):
    case CH_DEL:
        del ();
        return 1;
    case CH_CURS_LEFT:
    case CTRL ('B'):
        t_prev ();
        wantok = 0;
        break;
    case CH_CURS_RIGHT:
    case CTRL ('F'):
        t_next ();
        wantok = 0;
        break;
    case CH_CURS_UP:
    case CTRL ('P'):
        up (1);
        break;
    case CH_CURS_DOWN:
    case CTRL ('N'):
        down (1);
        break;
    case CH_HOME:
    case CTRL ('A'):
        t_bol ();
        wantok = 0;
        break;
    case CH_END:
    case CTRL ('E'):
        t_eol ();
        wantok = 0;
        break;
    case CH_PAGE_UP:
    case CTRL ('Y'):
        page (-1);
        break;
    case CH_PAGE_DOWN:
    case CTRL ('V'):
        page (1);
        break;
    case CTRL ('K'):
        cut ();
        cutting = 2;
        break;
    case CTRL ('U'):
        paste ();
        break;
    case CTRL ('W'):
        find (1, 1);
        break;
    case CTRL ('O'):
        save (1);
        break;
    case CTRL ('S'):
        save (0);
        break;
    case CTRL ('R'):
        openfile ();
        break;
    case CTRL ('X'):
        leave ();
        break;
    case CTRL ('G'):
    case CH_F1:
        s_help ();
        break;
    case CTRL ('L'):
        s_all ();
        break;
    case CH_RESIZE:
        s_resize ();
        break;
    default:
        if (k == '\t' || (k >= ' ' && k < 127)) {
            typed (k);
            return 1;
        }
        s_msg ("not a key edit knows (^G: the keys)");
    }
    return 0;
}

int main (int argc, char* argv[])
{
    struct hy_regs r;
    unsigned char i, k;
    int b;

    r.r[0] = (unsigned) quiet;                          /* Notes (Ctrl-C) ignored */
    hy_call (HY_NOTIFY, &r);
    if ((b = hy_banks_alloc (1)) < 0 || s_init ()) {
        write (2, "edit: no room (no RAM bank)\n", 28);
        return 1;
    }
    sbank = b;
    for (i = 1; i < argc; ++i) {
        open_doc (argv[i]);
    }
    if (!ndocs && open_doc (0)) {
        s_done ();
        return 1;
    }
    if (cur) {
        show (0);
    }
    while (!quit) {
        if (!kbhit ()) {
            s_render ();
        }
        k = cgetc ();
        s_msg ("");
        if (!command (k)) {
            D.ugroup = 0;
        }
        if (cutting) {                                  /* (^K and M-6 again: into the cut buffer together) */
            --cutting;
        }
    }
    s_done ();
    return 0;
}
