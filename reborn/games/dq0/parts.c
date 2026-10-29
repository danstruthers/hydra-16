/*
** parts.c - (overlay 6) a line typed in parts (term.c's enter): a line of several commands, "north then east", "open
** the door; go north", "take the sword and the shield", each part its own line in the queue, as if typed one after
** another (each carried out as the character's free); "north and east" one way (northeast), not two.  A part begun with
** a thing ("the shield", "my dagger") takes the verb of the part before it: "take the shield"
*/

#pragma code-name ("OV6CODE")
#pragma rodata-name ("OV6RODATA")

#include <string.h>
#include "dq0.h"

#define NPARTS      8
static const char ways[] = "n\0s\0e\0w\0ne\0nw\0se\0sw\0north\0south\0east\0west\0northeast\0northwest\0"
    "southeast\0southwest\0up\0down\0u\0d\0left\0right\0l\0r\0forward\0back\0f\0b\0ahead\0";
static const char dets[] = "the\0a\0an\0my\0your\0all\0both\0this\0that\0these\0those\0every\0each\0";
static const char units[] = "hour\0hours\0minute\0minutes\0";   /* (A duration's: "3 hours and 20 minutes" one) */
static const char nums[] = "a\0an\0one\0two\0three\0four\0five\0six\0seven\0eight\0nine\0ten\0";
static unsigned char pa[NPARTS], pn[NPARTS], np;    /* (The parts: where each begins in inl, its length) */

/* The word at inl + i one of list's (each 0-ended, an empty one last): 1 */
static unsigned char __fastcall__ among (unsigned char i, const char* list)
{
    static unsigned char n, k;

    for (n = 0; inl[i + n] && inl[i + n] != ' ' && inl[i + n] != ',' && inl[i + n] != ';'; ++n) {
    }
    for (; *list; list += k + 1) {
        k = strlen (list);
        if (k == n && !strnicmp (inl + i, list, n)) {
            return 1;
        }
    }
    return 0;
}

/* The part from a to b (its spaces and commas at each end left out), if there's anything in it */
static void __fastcall__ part (unsigned char a, unsigned char b)
{
    while (a < b && (inl[a] == ' ' || inl[a] == ',')) {
        ++a;
    }
    while (b > a && (inl[b - 1] == ' ' || inl[b - 1] == ',')) {
        --b;
    }
    if (b > a && np < NPARTS) {
        pa[np] = a;
        pn[np++] = b - a;
    }
}

/* The line typed in parts: at a ";", at "then" (", then", "and then"), at "and" (but between two ways, or a
** duration's two spans: "wait 3 hours and 20 minutes", the long durations, 2026-10-28) */
static void parts (void)
{
    static unsigned char i, a, w;

    np = 0;
    i = a = 0;
    if (alias_said (inl)) {
        i = strlen (inl);                       /* (A shortcut's own line whole: "alias x is north then dig") */
    }
    for (; inl[i]; ++i) {
        if (inl[i] == ';') {
            part (a, i);
            a = i + 1;
        } else if (inl[i] == ' ' && among (i + 1, "then\0\0")) {
            part (a, i);
            a = i + 5;
            i += 4;
        } else if (inl[i] == ' ' && among (i + 1, "and\0\0") && inl[i + 4] == ' ') {
            for (w = i; w > a && inl[w - 1] != ' ' && inl[w - 1] != ','; --w) {
            }
            if (among (i + 5, "then\0\0") || ((!among (w, ways) || !among (i + 5, ways)) && (!among (w, units) ||
                (!among (i + 5, nums) && (inl[i + 5] < '0' || inl[i + 5] > '9'))))) {
                part (a, i);            /* (Not "north and east": two commands; "and then" the "then"'s) */
                a = i + 5;
                i += 4;
            }
        }
    }
    part (a, i);
}

/* The line typed (inl) queued for t_line, a line for each of its parts.  0: no room (the line stays on the line
** typed, and Enter must be pressed again) */
unsigned char o_parts_in (void)
{
    static unsigned char k, v, vn, i;
    static unsigned need;

    parts ();
    for (need = k = 0; k < np; ++k) {
        need += pn[k] + 5 + (k ? pn[0] + 1 : 0);    /* (Room for the most a part can have: the verb before it too;
                                                    **   "help" for "?") */
    }
    if (!np) {
        need = 1;
    }
    if (qlen + need > QMAX) {
        return 0;
    }
    if (!np) {
        queue[qlen++] = 0;                          /* (An empty line: as before) */
    }
    for (k = vn = 0; k < np; ++k) {
        if (k && vn && among (pa[k], dets)) {       /* (A thing alone: the verb before it, "take the shield") */
            memcpy (queue + qlen, inl + v, vn);
            qlen += vn;
            queue[qlen++] = ' ';
        } else {                                    /* (Its own verb: the words before its thing, "pick up") */
            v = pa[k];
            for (i = 0, vn = 0; i < pn[k]; ++i) {
                if ((!i || inl[v + i - 1] == ' ') && among (v + i, dets)) {
                    vn = i ? i - 1 : 0;
                    break;
                }
                if (!vn && inl[v + i] == ' ') {
                    vn = i;                         /* (No thing's word: its first word) */
                }
            }
        }
        if (pn[k] == 1 && inl[pa[k]] == '?') {
            strcpy (queue + qlen, "help");          /* ("?": the help, the parser having no "?") */
            qlen += 5;
            continue;
        }
        memcpy (queue + qlen, inl + pa[k], pn[k]);
        qlen += pn[k];
        queue[qlen++] = 0;
    }
    return 1;
}
