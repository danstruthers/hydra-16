/*
** said.c - (overlay 27) the outcome events' words: the UI's one renderer (ENGINE-CRITIQUE.md's §3.2, narration as
** data).  The world tells what came of an action as an outcome event (msgs.def's: its kind the action, E_DIG, E_CLIMB,
** E_PORTAL, E_TRAP; E_OUTCOME its head, the outcome, n1, n2, the thing done to and the thing done with), and its words
** are a row of the phrase table (said.txt, packed by the build: obj/gen/dq0said.h), chosen by its kind and its outcome
** (and a field's value, a row's condition), its text said as it reads: the event's fields, the UI's words for its
** things (said_s), "you" or the denizen, articles, a heading from the way the character faces, the theme's names.
** What the UI works out for a row (which of a dig's refusals, a tool's name) its caller's (main.c's s_news); the words
** here and in said.txt alone.
*/

#pragma code-name ("OV27CODE")
#pragma rodata-name ("OV27RODATA")

#include <string.h>
#include <stdlib.h>
#include "dq0.h"
#include "hub.h"
#define SAID_TABLE
#include "dq0said.h"

/* A text's codes (saidgen.js's SC: the letters as they are, '\n' a new line; the others under SC_END the words most
** said, said_dw's) */
enum { SC_NL = 0x01, SC_END = 0x1E, SC_ALT = 0x1F, SC_NUM = 0x80, SC_CHOOSE = 0xA0, SC_CHANGED = 0xC0, SC_STR = 0xE0,
       SC_S = 0xE8, SC_A, SC_LIGHT, SC_FORGET, SC_ARG, SC_IF = 0xF0, SC_ROOM = 0xF8, SC_TRAP, SC_REL, SC_PIECE };
#define SR_ONCE         1                       /* (A row's flag: said as a look's sentences, none twice an answer) */

/* The UI's words for a row (the main RAM's: its callers set them) */
const char* said_s[3];                          /* The thing done to, the thing done with, a word of the row's own */
unsigned char said_n;                           /* A choice of the UI's for the row ({arg}) */
static unsigned char said_last[SAID_NKINDS];    /* Each kind's field last told ({F!...}: a dig's feet to go) */

#pragma bss-name (push, "OV27BSS")              /* (The bank's own, after its code: written as it goes) */
static const struct msg* sm;                    /* The event being worded */
static char* sp;                                /* lb's end */
static unsigned char sk;                        /* Its kind's place (said_kind's) */
static unsigned art;                            /* An "a" waiting for its word: its place in lb, plus 1; 0 none */
static unsigned char once;                      /* The row's said as a look's (lb_said) */
static char an[17];                             /* A denizen's name */
#pragma bss-name (pop)

/* Field f's value (said_f's: its byte, shift and mask; less K, more than K, times K) */
static unsigned __fastcall__ fv (unsigned char f)
{
    const unsigned char* d = said_f + f * 5;
    unsigned char v = ((const unsigned char*) sm)[d[0]] >> d[1] & d[2];

    switch (d[3]) {
    case 1:
        return v > d[4] ? v - d[4] : 0;
    case 2:
        return v > d[4];
    case 3:
        return v * d[4];
    }
    return v;
}

/* Past the code at p, and its bytes */
static const unsigned char* __fastcall__ step (const unsigned char* p)
{
    unsigned char c = *p++;

    return c == SC_TRAP ? p + 2 : c >= SC_ROOM ? p + 1 : p;
}

/* From an alternative's start to the SC_ALT or SC_END after it (the blocks in it passed over) */
static const unsigned char* __fastcall__ over (const unsigned char* p)
{
    unsigned char d = 0, c;

    for (;; p = step (p)) {
        c = *p;
        if (c == SC_ALT || c == SC_END) {
            if (!d) {
                return p;
            }
            if (c == SC_END) {
                --d;
            }
        } else if ((c >= SC_CHOOSE && c < SC_STR) || (c >= SC_IF && c < SC_ROOM)) {
            ++d;
        }
    }
}

/* lb's words so far said (an "a" made "an" first, if its word begins with a vowel), lb begun again */
static void flush (void)
{
    *sp = 0;
    if (art && strchr ("aeiou", lb[art + 1])) {
        memmove (lb + art + 1, lb + art, strlen (lb + art) + 1);
        lb[art] = 'n';
    }
    art = 0;
    if (lb[0]) {
        if (once) {
            lb_said ();
        } else {
            say (lb);
        }
    }
    sp = lb;
}

/* lb's end, after a word another's added to it (theme_named's, trap_name's) */
static void ended (void)
{
    sp = lb + strlen (lb);
}

/* A heading h's words, from the way the character faces ("ahead to your left") */
static void __fastcall__ rel (unsigned char h)
{
    unsigned char f;

    hub_mem (0, &f, &HUB->pc_face, 1);
    h = h - f & 7;
    strcpy (sp, h == 0 || h == 1 || h == 7 ? "ahead" : h >= 3 && h <= 5 ? "behind you" : "");
    if (h != 0 && h != 4) {
        strcat (sp, *sp ? (h < 4 ? " to your right" : " to your left") : (h < 4 ? "to your right" : "to your left"));
    }
    ended ();
}

/* A text said, from p to its end (0), or to the SC_ALT or SC_END that ends its alternative: there */
static const unsigned char* __fastcall__ run (const unsigned char* p)
{
    unsigned char c, v;
    const unsigned char* q;
    const char* s;

    for (;;) {
        c = *p;
        if (!c || c == SC_ALT || c == SC_END) {
            return p;
        }
        ++p;
        if (c < SC_NUM) {
            if (c == SC_NL) {
                flush ();
                msg ("");                       /* (A line of its own: msg's) */
            } else if (c < ' ' && c != '\n') {
                strcpy (sp, said_dw + said_do[c < '\n' ? c - 2 : c - 4]);    /* (A word most said) */
                sp += strlen (sp);
            } else {
                *sp++ = c;
            }
            continue;
        }
        if (c < SC_CHOOSE) {
            utoa (fv (c - SC_NUM), sp, 10);
            sp += strlen (sp);
            continue;
        }
        if (c < SC_CHANGED || (c >= SC_IF && c < SC_ROOM)) {    /* (A choice: by a field, a word given, the UI's number) */
            v = c >= SC_IF ? (c == SC_IF + 4 ? said_n : c == SC_IF + 3 ? sm->who != 0 : said_s[c - SC_IF] &&
                *said_s[c - SC_IF]) : fv (c - SC_CHOOSE) > 255 ? 255 : fv (c - SC_CHOOSE);
            for (; v; --v) {
                q = over (p);
                if (*q == SC_END) {
                    break;                      /* (The last for any past it) */
                }
                p = q + 1;
            }
            for (p = run (p); *p != SC_END; p = over (p + 1)) {
            }
            ++p;
            continue;
        }
        if (c < SC_STR) {                       /* (Said only if the field's not as it was last told) */
            v = fv (c - SC_CHANGED);
            if (v != said_last[sk]) {
                said_last[sk] = v;
                p = run (p);
            } else {
                p = over (p);
            }
            ++p;
            continue;
        }
        if (c < SC_S) {                         /* (A word the UI's given, or the actor: capitalised, its odd one) */
            v = c - SC_STR;
            if (v >= 6) {
                if (!sm->who) {
                    s = "you";
                } else {
                    hub_mem (0, an + 4, HUB->actor[sm->who - 1].name, 12);
                    an[16] = 0;
                    s = an + 4;
                    if (an[4] >= 'a') {
                        memcpy (an, "the ", 4);
                        s = an;
                    }
                }
            } else {
                s = said_s[v >> 1];
            }
            if (s) {
                strcpy (sp, s);
                if (v & 1 && *sp >= 'a' && *sp <= 'z') {
                    *sp -= 'a' - 'A';
                }
                sp += strlen (sp);
            }
            continue;
        }
        switch (c) {
        case SC_S:
            if (sm->who) {
                *sp++ = 's';
            }
            break;
        case SC_A:
            art = sp - lb + 1;
            *sp++ = 'a';
            break;
        case SC_LIGHT:
            *sp = 0;
            theme_named (TN_LIGHT, 0);          /* (Added to lb: theme.c's) */
            ended ();
            break;
        case SC_FORGET:
            said_last[sk] = 0;
            break;
        case SC_ARG:
            utoa (said_n, sp, 10);
            sp += strlen (sp);
            break;
        case SC_ROOM:
            *sp = 0;
            theme_named (TN_ROOM, fv (*p++ - 1));
            ended ();
            break;
        case SC_TRAP:
            *sp = 0;
            trap_name (fv (p[0] - 1), fv (p[1] - 1), 0);    /* (Added to lb: words.c's) */
            p += 2;
            ended ();
            break;
        case SC_REL:
            rel (fv (*p++ - 1));
            break;
        case SC_PIECE:
            run (said_rows + said_pc[*p++ - 1]);    /* (A piece's text, said.txt's @NAME) */
            break;
        }
    }
}

/* Outcome event m worded: its kind's row for its outcome (the first whose condition holds, or the one with none), said.
** None, nothing said */
void __fastcall__ o_outcome_words (const struct msg* m)
{
    const unsigned char* p;
    unsigned char v;

    sm = m;
    for (sk = 0; said_kind[sk] != m->kind; ) {
        if (++sk == SAID_NKINDS) {
            return;
        }
    }
    for (p = said_rows + said_at[sk]; *p != 0xFF; p += strlen ((const char*) p + 4) + 5) {
        if (*p == E_OUTCOME_OUTCOME (*m) && (p[1] == 0xFF || ((v = fv (p[1])) < 8 && p[2] & 1 << v))) {
            once = p[3] & SR_ONCE;
            sp = lb;
            art = 0;
            run (p + 4);
            flush ();
            return;
        }
    }
}
