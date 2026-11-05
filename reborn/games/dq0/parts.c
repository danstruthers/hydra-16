/*
** parts.c - (overlay 6) a line typed in parts (term.c's enter): a line of several commands, "north then east", "open
** the door; go north", "take the sword and the shield", "drop the sack, the knife and the rope", each part its own line
** in the queue, as if typed one after another (each carried out as the character's free); "north and east" one way
** (northeast), not two; nothing within quotes.
**
** A list (2026-11-04: HOUSE.md's lists): a part begun with a thing ("the shield", "my dagger") takes the verb of the part
** before it: "take the shield".  One with no word to say it's a thing ("take rope and torch") is carried out if it's a
** command ("take the rope and look"); no verb in it, the parser's (parts_op's PO_AGAIN, from the command), the verb's put
** to it then.  What's said after the last goes for all of them where it's wanted: "put the rope and the torch in the
** sack", "give the rope and the knife to the dwarf"; "take and drop the pick", the last's thing for each.  Stopped, the
** rest of the line goes with it (PO_STOP, as a long task's stopped or asked about: task.c's; "stop" typed, here): lines
** typed after it are kept, as they were.
*/

#pragma code-name ("OV6CODE")
#pragma rodata-name ("OV6RODATA")
#pragma data-name ("OV6RODATA")

#include <string.h>
#include "dq0.h"
#include "hub.h"

#define NPARTS      8
static const char ways[] = "n\0s\0e\0w\0ne\0nw\0se\0sw\0north\0south\0east\0west\0northeast\0northwest\0"
    "southeast\0southwest\0up\0down\0u\0d\0left\0right\0l\0r\0forward\0back\0f\0b\0ahead\0";
static const char dets[] = "the\0a\0an\0my\0your\0all\0both\0this\0that\0these\0those\0every\0each\0";
static const char units[] = "hour\0hours\0minute\0minutes\0";   /* (A duration's: "3 hours and 20 minutes" one) */
static const char nums[] = "a\0an\0one\0two\0three\0four\0five\0six\0seven\0eight\0nine\0ten\0";
static const char ups[] = "up\0down\0off\0on\0away\0out\0";     /* (A verb's second word: "pick up", "take off") */
static const char preps[] = "in\0into\0inside\0on\0onto\0to\0from\0at\0under\0";
static const char wants[] = "put\0place\0give\0hand\0offer\0throw\0toss\0hurl\0";   /* (Verbs with no meaning without
                                                **   what's said after the thing) */
static const char chain[] = "take\0get\0drop\0open\0close\0hold\0stow\0wear\0light\0";  /* ("take and drop the pick") */

/* The line's own, this overlay's data (the main RAM's room): where each part begins in inl, its length, it follows a
** list's word ("and", a comma); so many parts */
static unsigned char pa[NPARTS] = { 0 }, pn[NPARTS] = { 0 }, ps[NPARTS] = { 0 }, np = 0;

/* A list kept for its later parts (a part with no verb in it, PO_AGAIN): its line, the verb before each (where in lc, its
** length), so many parts; and qafter, the queue's last bytes that aren't its, typed since (none of it left, qlen no more
** than them) */
static char lc[INMAX + 1] = { 0 };
static unsigned char lv[NPARTS] = { 0 }, lvn[NPARTS] = { 0 }, lnp = 0;
static unsigned qafter = 0;

static unsigned char i = 0, a = 0, k = 0, v = 0, vn = 0, sep = 0;

/* The word at inl + at one of list's (each 0-ended, an empty one last): 1 */
static unsigned char __fastcall__ among (unsigned char at, const char* list)
{
    static unsigned char n = 0, m = 0;
    const char* w = inl + at;

    for (n = 0; w[n] && w[n] != ' ' && w[n] != ',' && w[n] != ';'; ++n) {
    }
    for (; *list; list += m + 1) {
        m = strlen (list);
        if (m == n && !strnicmp (w, list, n)) {
            return 1;
        }
    }
    return 0;
}

/* A part ended at i (its spaces and commas at each end left out), if there's anything in it; the next begun past the
** separator (its skip letters, s: a list's) */
static void __fastcall__ cut (unsigned char skip)
{
    static unsigned char b = 0;

    b = i;
    while (a < b && (inl[a] == ' ' || inl[a] == ',')) {
        ++a;
    }
    while (b > a && (inl[b - 1] == ' ' || inl[b - 1] == ',')) {
        --b;
    }
    if (b > a && np < NPARTS) {
        pa[np] = a;
        pn[np] = b - a;
        ps[np++] = sep;
    }
    a = i + skip;
    i += skip - 1;
}

/* The line typed in parts: at a ";", at "then" (", then", "and then"), at a comma, at "and" (but between two ways, or a
** duration's two spans: "wait 3 hours and 20 minutes", the long durations, 2026-10-28); within quotes, none */
static void parts (void)
{
    static unsigned char q = 0, w = 0, c = 0;

    np = i = a = q = sep = 0;
    if (alias_said (inl)) {
        i = strlen (inl);                       /* (A shortcut's own line whole: "alias x is north then dig") */
    }
    for (; (c = inl[i]) != 0; ++i) {
        if (c == '"') {
            q ^= 1;
        } else if (!q) {
            if (c == ';' || c == ',') {
                cut (1);
                sep = c == ',';
            } else if (c == ' ') {
                if (among (i + 1, "then\0\0")) {
                    cut (5);
                    sep = 0;
                } else if (among (i + 1, "and\0\0") && inl[i + 4] == ' ') {
                    for (w = i; w > a && inl[w - 1] != ' ' && inl[w - 1] != ','; --w) {
                    }
                    if (among (i + 5, "then\0\0")) {
                        cut (5);                /* ("and then" the "then"'s) */
                    } else if ((!among (w, ways) || !among (i + 5, ways)) && (!among (w, units) ||
                        (!among (i + 5, nums) && (inl[i + 5] < '0' || inl[i + 5] > '9')))) {
                        cut (5);                /* (Not "north and east": two commands) */
                        sep = 1;
                    }
                }
            }
        }
    }
    cut (1);
}

/* Part p's own verb, the words before its thing ("pick up the sword": "pick up"; "take rope": "take"): its length */
static unsigned char __fastcall__ verb_of (unsigned char p)
{
    static unsigned char b = 0, n = 0, j = 0;

    b = pa[p];
    n = pn[p];
    for (j = 0; j < n; ++j) {
        if ((!j || inl[b + j - 1] == ' ') && among (b + j, dets)) {
            return j ? j - 1 : 0;
        }
    }
    for (j = 0; j < n && inl[b + j] != ' '; ++j) {
    }
    if (j < n && among (b + j + 1, ups)) {
        for (++j; j < n && inl[b + j] != ' '; ++j) {
        }
    }
    return j;
}

/* What's said after part p's thing (from a word of preps's): where in inl (its length to its part's end, tln), or 0 */
static unsigned char tln = 0;
static unsigned char __fastcall__ tail_of (unsigned char p)
{
    static unsigned char j = 0;

    tln = 0;
    for (j = verb_of (p) + 1; j < pn[p]; ++j) {
        if (inl[pa[p] + j - 1] == ' ' && among (pa[p] + j, preps)) {
            tln = pn[p] - j;
            return pa[p] + j;
        }
    }
    return 0;
}

/* The rest of the list out of the queue (lines typed since kept) */
static void let_go (void)
{
    if (qlen > qafter) {
        memmove (queue, queue + (qlen - qafter), qafter);
        qlen = qafter;
        held |= 2;                              /* (The next line's parse, posted, with it: main's) */
    }
    lnp = 0;
}

/* n bytes of s to the queue */
static void __fastcall__ q_put (const char* s, unsigned char n)
{
    memcpy (queue + qlen, s, n);
    qlen += n;
}

/* The line typed (inl) queued for t_line, a line for each of its parts.  0: no room (the line stays on the line
** typed, and Enter must be pressed again) */
static unsigned char __fastcall__ parts_op (unsigned char op);
unsigned char __fastcall__ o_parts_in (unsigned char op)
{
    static unsigned qwas = 0;
    static unsigned char own = 0, j = 0, t = 0;

    if (op) {
        return parts_op (op);
    }
    parts ();
    a = qlen > qafter;                          /* (A list's parts still to come) */
    if (a && np && pn[0] == 4 && !strnicmp (inl + pa[0], "stop", 4)) {
        let_go ();                              /* ("stop" at once, not after them) */
        a = 0;
    }
    qwas = qlen;
    if (np > 1) {
        lnp = np;
        strcpy (lc, inl);
    }
    if (!np) {
        if (qlen == QMAX) {
            return 0;
        }
        queue[qlen++] = 0;                      /* (An empty line: as before) */
    }
    for (k = v = vn = 0; k < np; ++k) {
        if (qlen + pn[k] + 8 + vn > QMAX) {
            qlen = qwas;
            return 0;
        }
        if (np > 1) {
            lv[k] = v;                          /* (The verb before it, a later "rope"'s) */
            lvn[k] = ps[k] ? vn : 0;
        }
        own = np > 1 ? verb_of (k) : 0;         /* (A line of one part: none wanted, its answer the sooner) */
        if (k && vn && among (pa[k], dets)) {   /* (A thing alone: the verb before it, "take the shield") */
            q_put (inl + v, vn);
            queue[qlen++] = ' ';
        } else if (!ps[k] || pn[k] != own) {    /* (One word in a list, no verb said: "take rope and torch") */
            v = pa[k];
            vn = own;
        }
        if (pn[k] == 1 && inl[pa[k]] == '?') {
            q_put ("help", 5);                  /* ("?": the help, the parser having no "?") */
            continue;
        }
        q_put (inl + pa[k], pn[k]);
        /* (What's wanted of the list's next parts: "put the rope and the torch in the sack"; "take and drop the pick") */
        t = 0;
        if (np > 1) {
            for (j = k + 1; j < np && ps[j]; ++j) {
            }
        } else {
            j = k + 1;
        }
        if (--j > k) {
            if (pn[k] == own && among (pa[k], chain) && (t = verb_of (k + 1)) != 0) {
                tln = pn[k + 1] > t ? pn[k + 1] - t - 1 : 0;
                t += pa[k + 1] + 1;
            } else if (!tail_of (k) && (t = tail_of (j)) != 0 && !among (pa[j], dets) && !among (v, wants)) {
                t = 0;
            }
        }
        if (t && tln && qlen + tln + 2 <= QMAX) {
            queue[qlen++] = ' ';
            q_put (inl + t, tln);
        }
        queue[qlen++] = 0;
    }
    if (np > 1) {                               /* (A list now) */
        qafter = 0;
    } else if (a) {
        qafter += qlen - qwas;                  /* (Typed after a list: not its) */
    } else {
        qafter = qlen;                          /* (No list: nothing's let go) */
        lnp = 0;
    }
    return 1;
}

/* A list's matters (parts_in's): PO_STOP, the long task stopped, or asked about: the rest of its line let go.  PO_AGAIN, the command cl
** has no verb, and no way in it ("rope"): a list's later part (after "and", or a comma) has the verb of the part before it
** put to it, back in the queue: 1 (nothing to say) */
static unsigned char __fastcall__ parts_op (unsigned char op)
{
    static unsigned d = 0;
    static unsigned char n = 0, m = 0;

    if (op == PO_STOP) {
        let_go ();
        return 0;
    }
    if (!(cmd.flags & (CF_NOVERB | CF_STRANGE)) || !lnp || qlen < qafter || cmd.dir != 0xFF) {
        return 0;
    }
    d = qlen - qafter;                          /* (Its parts still to come, counted: which this is) */
    for (n = 0; d; ) {
        n += !queue[--d];
    }
    if (n >= lnp) {
        return 0;
    }
    n = lnp - 1 - n;
    m = lvn[n];
    if (!m || !strnicmp (cl, lc + lv[n], m)) {
        return 0;
    }
    n = lv[n];
    d = m + 2 + strlen (cl);
    if (qlen + d > QMAX) {
        return 0;
    }
    memmove (queue + d, queue, qlen);
    memcpy (queue, lc + n, m);
    queue[m] = ' ';
    strcpy (queue + m + 1, cl);
    qlen += d;
    return 1;
}
