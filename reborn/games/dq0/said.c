/*
** said.c - (overlay 27) the outcome events' words: the UI's one renderer (ENGINE-CRITIQUE.md's §3.2, narration as
** data).  The world tells what came of an action as an outcome event (msgs.def's: its kind the action, E_DIG, E_CLIMB,
** E_PORTAL, E_TRAP; E_OUTCOME its head, the outcome, n1, n2, the thing done to and the thing done with), and its words
** are a row of the phrase table (said.txt, packed by the build: obj/gen/dq0said.h), chosen by its kind and its outcome
** (and a field's value, a row's condition), its text said as it reads: the event's fields, the UI's words for its
** things (said_s), "you" or the denizen, articles, a heading from the way the character faces, the theme's names.
** What the UI works out for a row (which of a dig's refusals, a tool's name) its caller's (main.c's s_news); the words
** here and in said.txt alone.  The table's rows are overlay 28's (a bank of their own, 2026-11-11: this one's room
** for the renderer and the kinds to come), read a byte at a time through bank.s's said_b; the rest here.
*/

#pragma code-name ("OV27CODE")
#pragma rodata-name ("OV27RODATA")

#include <string.h>
#include <stdlib.h>
#include "dq0.h"
#include "hub.h"
#define SAID_TABLE
#include "dq0said.h"

unsigned char __fastcall__ said_b (const unsigned char* p);    /* (bank.s's: byte p of overlay 28's bank, the rows') */
#define RB(p)           said_b (p)

/* A text's codes (saidgen.js's SC: the letters as they are, '\n' a new line; the others under SC_END the words most
** said, said_dw's) */
enum { SC_NL = 0x01, SC_END = 0x1E, SC_ALT = 0x1F, SC_POSS = 0x7F, SC_NUM = 0x80, SC_CHOOSE = 0xA0, SC_CHANGED = 0xC0,
       SC_STR = 0xE0, SC_S = 0xE8, SC_A, SC_LIGHT, SC_FORGET, SC_ARG, SC_SING, SC_WORD, SC_ITS, SC_IF = 0xF0, SC_ROOM = 0xF8,
       SC_TRAP, SC_REL,
       SC_PIECE,
       SC_KIND, SC_NAME, SC_STR2, SC_EXT };      /* (SC_EXT X: the look's lists, said.txt's {list} ..., 2026-11-16) */
#define SC_FX           31                      /* (SC_NUM, SC_CHOOSE, SC_CHANGED + 31: the field's a byte after it, the 32nd
                                                **   on: the table's fields past 31, 2026-11-12) */
#define SR_ONCE         1                       /* (A row's flag: said as a look's sentences, none twice an answer) */

/* The UI's words for a row (the main RAM's: its callers set them) */
const char* said_s[6];                          /* The thing done to, the thing done with, a word of the row's own;
                                                **   three more ({w4} {w5} {w6}: a blow's names and lines, 2026-11-12) */
unsigned char said_n;                           /* A choice of the UI's for the row ({arg}) */
unsigned char said_q;                           /* The words left in lb, not said (a denizen's sound's words, heard:
                                                **   its caller's to use, 2026-11-11) */
const struct li* said_li;                       /* The UI's list ({list}: its frame's a entries; the look's lists,
                                                **   2026-11-16) */
char* said_to = lb;                             /* Where in lb the words go (said_q's: the world's things' words, each
                                                **   after the text before it, tt's: things.c's, 2026-11-12) */
static unsigned char said_last[SAID_NKINDS];    /* Each kind's field last told ({F!...}: a dig's feet to go) */

#pragma bss-name (push, "OV27BSS")              /* (The bank's own, after its code: written as it goes) */
static const struct msg* sm;                    /* The event being worded */
static char* sp;                                /* lb's end */
static unsigned char sk;                        /* Its kind's place (said_kind's) */
static unsigned char ac;                        /* Its actor: 0 the character, else the actor plus 1 (its who, or its
                                                **   kind's own field, said_ka's: E_BEAT's whose, 2026-11-11) */
static unsigned art;                            /* An "a" waiting for its word: its place in lb, plus 1; 0 none */
static unsigned char once;                      /* The row's said as a look's (lb_said) */
#pragma bss-name (pop)

/* Actor n - 1 one of the speaking peoples (the hub's folk, kinds.txt's nature f: dwarves, elves, men, kobolds, the
** gargun), said "they" ("their", "them"): 1; the character (n 0), a beast, the dead, "it": 0 (the user's, 2026-11-17).
** n with 0x40: one not seen, "something", "it" (UK_FIGHT's actor, blows.c's) */
static unsigned char __fastcall__ they (unsigned char n)
{
    unsigned char f = 0;

    if (n && n < 0x40) {
        hub_mem (0, &f, &HUB->folk[(n - 1) & 7], 1);
    }
    return f;
}

/* Field f's value (said_f's: its byte, shift and mask; less K, more than K, times K; the actor it names a person) */
static unsigned __fastcall__ fv (unsigned char f)
{
    const unsigned char* d = said_f + f * 5u;
    unsigned char v = ((const unsigned char*) sm)[d[0]] >> d[1] & d[2];

    switch (d[3]) {
    case 4:
        return *(const unsigned*) ((const unsigned char*) sm + d[0]);  /* (A word whole: a count, the look's lists') */
    case 5:
        return they (v);                        /* (Its actor plus 1 a person: {they F?it|they}) */
    case 1:
        return v > d[4] ? v - d[4] : 0;
    case 2:
        return v > d[4];
    case 3:
        return v * d[4];
    }
    return v;
}

/* Actor n - 1's name, at lb's end and 4 on (room for "the " before it).  (Into lb, the main RAM: hub_mem's bytes go
** between the hub's bank in the window and the main RAM, not this bank's; stage 1's name in OV27BSS was written into
** the hub's header, 2026-11-11) */
static char* __fastcall__ name (unsigned char n)
{
    hub_mem (0, sp + 4, HUB->actor[(n - 1) & 7].name, 12);
    sp[16] = 0;
    return sp + 4;
}

/* From an alternative's start (its length, a byte: saidgen.js's, 2026-11-16) to the SC_ALT or SC_END after it, in a
** read (each byte through said_b before: a block's alternatives some 100 cycles a byte, a long choice's tens of
** thousands) */
#define over(p)         ((p) + 1 + RB (p))

/* An "a" waiting made "an", if its word begins with a vowel (a list's each, as it's said: li's) */
static void an (void)
{
    *sp = 0;
    if (art && strchr ("aeiou", lb[art + 1])) {
        memmove (lb + art + 1, lb + art, strlen (lb + art) + 1);
        lb[art] = 'n';
        ++sp;
    }
    art = 0;
}

/* lb's words so far said (an "a" made "an" first, if its word begins with a vowel), lb begun again */
static void flush (void)
{
    an ();
    if (lb[0] && !said_q) {
        if (once) {
            lb_said ();
        } else {
            say (lb);
        }
    }
    sp = said_to;
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

/* The row of kind sk for outcome o (the first whose condition holds, or the one with none): its start; 0 none */
static const unsigned char* __fastcall__ row (unsigned char o)
{
    const unsigned char* p;
    unsigned char v, n, c;

    for (p = said_rows + said_at[sk]; (n = RB (p)) != 0; p += n) {     /* (Each row its length first) */
        v = RB (p + 1);
        if (v != o && v != SAID_ANY) {
            continue;
        }
        if ((c = RB (p + 2)) != 0xFF) {         /* (Its condition: a field's one value, or the mask of its values) */
            v = fv (c & 0x7F);
            if (c & 0x80 ? v != RB (p + 3) : v >= 8 || !(RB (p + 3) & 1 << v)) {
                continue;
            }
        }
        return p;
    }
    return 0;
}

/* A kind's word made more than one ({plural}: "iron pots", "heaps of sand", its first word's before " of "; {people}, p
** 1: the whole, "dwarves"), or one ({single}, p 2: one of a lot's, "rations" a "ration", "pennies" a "penny", as the
** world's many words it), the text given (said_s[2]) */
static void __fastcall__ plural (unsigned char p)
{
    const char* n = said_s[2];
    unsigned char k, c;

    if (p == 2) {
        strcpy (sp, n);
        if ((k = strlen (sp)) > 3) {
            c = sp[k - 3] == 'i' && sp[k - 2] == 'e';
            strcpy (sp + k - (c ? 3 : 1), c ? "y" : "");
        }
        sp += strlen (sp);
        return;
    }
    if (p) {
        k = strlen (n);
    } else {
        for (k = 0; n[k] && (n[k] != ' ' || n[k + 1] != 'o' || n[k + 2] != 'f' || n[k + 3] != ' '); ++k) {
        }
    }
    c = k ? n[k - 1] : 0;
    memcpy (sp, n, k);
    sp += k;
    if (c == 'y' && k > 1 && !strchr ("aeiou", n[k - 2])) {
        strcpy (sp - 1, "ies");
    } else if (p && c == 'f') {
        strcpy (sp - 1, "ves");                 /* ("dwarves", "elves": the peoples, 2026-10-29) */
    } else {
        strcpy (sp, c == 's' || c == 'x' || c == 'z' || (c == 'h' && k > 1 && strchr ("cs", n[k - 2])) ? "es" : "s");
    }
    strcat (sp, n + k);
    sp += strlen (sp);
}

static const unsigned char* __fastcall__ run (const unsigned char* p);

/* The UI's list (said_li, the frame's a of them), each its UK_LI row by its li ("two iron pots", "a giant rat"; its
** text its name, w4 its capital), joined (j: 0 ", " and " and " the last; 1 "; "; 2 ", ": said.txt's LI rows too):
** the look's lists, one renderer for them (the user's, 2026-11-16) */
static void __fastcall__ li (unsigned char j)
{
    const struct msg* f = sm;
    const struct li* e = said_li;
    unsigned char k = sk, i, n = f->a;

    for (sk = 0; said_kind[sk] != UK_LI; ++sk) {
    }
    for (i = 0; i < n; ++i, ++e) {
        if (i) {
            run (row (j == 1 ? LI_SEMI : j == 2 || i + 1 < n ? LI_COMMA : LI_AND) + 5);
        }
        sm = (const struct msg*) e;
        said_s[2] = e->name;
        said_s[3] = e->cap;
        run (row (fv (said_ko[sk])) + 5);
        an ();
    }
    sm = f;
    sk = k;
}

/* A text said, from p to its end (0), or to the SC_ALT or SC_END that ends its alternative: there */
static const unsigned char* __fastcall__ run (const unsigned char* p)
{
    unsigned char c, v, f;
    const unsigned char* q;
    const char* s;

    for (;;) {
        c = RB (p);
        if (!c || c == SC_ALT || c == SC_END) {
            return p;
        }
        ++p;
        if (c < SC_NUM) {
            if (c == SC_NL) {
                flush ();
                msg ("");                       /* (A line of its own: msg's) */
            } else if (c == SC_POSS) {          /* (The actor's, after it: "Your", "The bandit's") */
                strcpy (sp, ac ? "'s" : "r");
                sp += strlen (sp);
            } else if (c < ' ' && c != '\n') {
                strcpy (sp, said_dw + said_do[c < '\n' ? c - 2 : c - 4]);    /* (A word most said) */
                sp += strlen (sp);
            } else {
                *sp++ = c;
            }
            continue;
        }
        if (c < SC_STR && (c & 0x1F) == SC_FX) {
            f = RB (p);                         /* (Past the 31st field: its byte) */
            ++p;
        } else {
            f = c & 0x1F;
        }
        if (c < SC_CHOOSE) {
            utoa (fv (f), sp, 10);
            sp += strlen (sp);
            continue;
        }
        if (c < SC_CHANGED || (c >= SC_IF && c < SC_ROOM)) {    /* (A choice: by a field, a word given, the UI's number) */
            if (c >= SC_IF) {
                v = c - SC_IF;
                v = v == 4 ? said_n : v == 3 ? ac != 0 : (s = said_s[v > 4 ? v - 2 : v]) && *s;
            } else {
                v = fv (f) > 255 ? 255 : fv (f);
            }
            for (; v; --v) {
                q = over (p);
                if (RB (q) == SC_END) {
                    break;                      /* (The last for any past it) */
                }
                p = q + 1;
            }
            for (p = run (p + 1); RB (p) != SC_END; p = over (p + 1)) {
            }
            ++p;
            continue;
        }
        if (c < SC_STR) {                       /* (Said only if the field's not as it was last told) */
            v = fv (f);
            if (v != said_last[sk]) {
                said_last[sk] = v;
                p = run (p + 1);
            } else {
                p = over (p);
            }
            ++p;
            continue;
        }
        if (c < SC_S || c == SC_STR2) {         /* (A word the UI's given, or the actor: capitalised, its odd one; its
                                                **   fourth to sixth, SC_STR2's, the byte after it) */
            if (c == SC_STR2) {
                v = RB (p);
                ++p;
                s = said_s[v >> 1];
            } else if ((v = c - SC_STR) >= 6) {
                if (!ac) {
                    s = "you";
                } else {
                    s = name (ac);
                    if (*s >= 'a') {
                        memcpy (sp, "the ", 4);
                        s = sp;
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
            if (ac) {
                *sp++ = 's';
            }
            break;
        case SC_ITS:                            /* (The actor's: "your", "its"; a person's "their", 2026-11-17) */
            strcpy (sp, !ac ? "your" : they (ac) ? "their" : "its");
            sp += strlen (sp);
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
        case SC_WORD:                           /* (A word said less often: the 27th on, a byte after it) */
            strcpy (sp, said_dw + said_do[25 + RB (p)]);
            ++p;
            sp += strlen (sp);
            break;
        case SC_SING:                           /* (The word before it made one: "1 arrow") */
            if (sp[-1] == 's') {
                --sp;
            }
            break;
        case SC_TRAP:
            *sp = 0;
            trap_name (fv (RB (p) - 1), fv (RB (p + 1) - 1), 0);    /* (Added to lb: words.c's) */
            p += 2;
            ended ();
            break;
        default:                                /* (A code and its field: SC_ROOM, SC_REL, SC_PIECE, SC_KIND, SC_NAME) */
            v = RB (p) - 1;
            ++p;
            switch (c) {
            case SC_ROOM:
                *sp = 0;
                theme_named (TN_ROOM, fv (v));
                ended ();
                break;
            case SC_REL:
                rel (fv (v));
                break;
            case SC_PIECE:
                run (said_rows + said_pc[v]);   /* (A piece's text, said.txt's @NAME) */
                break;
            case SC_KIND:                       /* (A thing's kind's name, the catalogue's) */
                hub_mem (KIND_BANK, sp, CATALOGUE[fv (v)].name, sizeof CATALOGUE[0].name);
                sp[sizeof CATALOGUE[0].name] = 0;
                ended ();
                break;
            case SC_NAME:                       /* (An actor's name as it is: the field its entry plus 1) */
                strcpy (sp, name (fv (v)));
                ended ();
                break;
            case SC_EXT:                        /* (The look's lists': a list, a plural, "A") */
                if (v < 3) {
                    li (v);
                } else if (v != 5) {
                    plural (v == 6 ? 2 : v - 3);
                } else {
                    art = sp - lb + 1;
                    *sp++ = 'A';
                }
                break;
            }
        }
    }
}

/* Outcome event m worded: its kind's row for its outcome (the first whose condition holds, or the one with none), said.
** None, nothing said */
void __fastcall__ o_outcome_words (const struct msg* m)
{
    const unsigned char* p;

    sm = m;
    for (sk = 0; said_kind[sk] != m->kind; ) {
        if (++sk == SAID_NKINDS) {
            return;
        }
    }
    ac = said_ka[sk] ? fv (said_ka[sk] - 1) : m->who;   /* (Its actor: its who, or the kind's own field) */
    if ((p = row (fv (said_ko[sk]))) != 0) {    /* (Its outcome: the head's, or the kind's own field, said.txt's kind) */
        once = RB (p + 4) & SR_ONCE;
        sp = said_to;
        art = 0;
        run (p + 5);
        flush ();
    }
}

/* A sentence with a list (UK_LIST, said_li its entries) worded into lb at lbe, not said (the look's: its words
** streamed by their callers' add, lbsaid.s's), lbe after it */
void __fastcall__ o_list_words (const struct msg* m)
{
    said_to = lbe;
    said_q = 1;
    o_outcome_words (m);
    said_q = 0;
    said_to = lb;
    lbe += strlen (lbe);
}
