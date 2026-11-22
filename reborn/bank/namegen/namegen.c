/*
** namegen.c - (a bank program: sdk/c/include/bankprog.h) names in a world's tongues (games/dq0/WORLDS.md): a person's, a
** place's or a people's, made of a tongue's sounds (its onsets, vowels and codas, and a people's ending: the caller's,
** the world's tongue line's) from a seed, the same name for the same seed every time (its own dice: its caller's go on
** as they were).  themegen's (the region, its peoples, their places and their great ones, a prisoner, a seeker: the
** world's tongues, its history.dat's); and from the shell, bankrun's: "bankrun namegen.bp TONGUE [N [SEED]]" says N
** names of each form in the first world's tongue (its tongues.h's, as the programs between dungeons have them).
*/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/* A name asked for: its seed, tongue (1 ...: its number, for the dice) and form, and the tongue's sounds (its onsets,
** its vowels, its codas, words a space between, and a people's ending); the name back in out */
struct nq {
    unsigned long   seed;
    unsigned char   tongue, form;
    char            out[16];
    const char* const* snd;
};
enum { NG_PERSON, NG_PLACE, NG_PEOPLE };

#include "../../games/dq0/worlds/first/tongues.h"   /* (The first world's: bankrun's, and the programs between dungeons') */

static unsigned long x;                         /* (Its dice: xorshift32, as dice.h's) */

static unsigned __fastcall__ roll (unsigned char n)
{
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (unsigned) (x >> 8) % n;
}

/* One of the words of s (spaces between), added to out (held under 15 letters) */
static void __fastcall__ pick (const char* s, char* out)
{
    unsigned char n = 1, k, len = strlen (out);
    const char* p;

    for (p = s; *p; ++p) {
        n += *p == ' ';
    }
    for (k = roll (n), p = s; k; ++p) {
        k -= *p == ' ';
    }
    while (*p && *p != ' ' && len < 14) {
        out[len++] = *p++;
    }
    out[len] = 0;
}

/* A doubled letter (but l, r, s, n, e), or three vowels together: a name that won't say well */
static unsigned char __fastcall__ bad (const char* o)
{
    unsigned char i, v = 0;

    for (i = 0; o[i]; ++i) {
        if (i && o[i] == o[i - 1] && !strchr ("lrsne", o[i])) {
            return 1;
        }
        v = strchr ("aeiou", o[i]) ? v + 1 : 0;
        if (v == 3) {
            return 1;
        }
    }
    return 0;
}

/* A name (q's tongue, form and seed): its length (4-8 letters; a people's 4-10) */
unsigned char __fastcall__ name (struct nq* q)
{
    const char* const* t = q->snd;
    unsigned char tries, n, syl, i, place = q->form == NG_PLACE;

    x = q->seed * 2654435761UL + q->tongue * 40503U + q->form + 1;
    if (!x) {
        x = 1;
    }
    for (tries = 0; tries < 16; ++tries) {
        q->out[0] = 0;
        if (q->form == NG_PEOPLE) {             /* (A syllable, and the tongue's ending: "Khal" "im") */
            pick (t[0], q->out);
            pick (t[1], q->out);
            pick (t[2], q->out);
            n = strlen (q->out);
            if (strchr ("aeiou", q->out[n - 1])) {
                q->out[n - 1] = 0;
            }
            strcat (q->out, t[3]);
        } else {
            syl = !place && !roll (4) ? 3 : 2;
            for (i = 0; i < syl; ++i) {         /* (Its onset: the first's, but one in six or eight begun with a
                                                **   vowel; a place's second's, one in two) */
                if (i ? !place || roll (2) : roll (place ? 6 : 8)) {
                    pick (t[0], q->out);
                }
                pick (t[1], q->out);
                if (i + 1 == syl || !roll (place ? 2 : 3)) {
                    pick (t[2], q->out);
                }
            }
        }
        n = strlen (q->out);
        if (n >= 4 && n <= (q->form == NG_PEOPLE ? 10 : 8) && !bad (q->out)) {
            break;
        }
    }
    q->out[10] = 0;
    q->out[0] &= ~0x20;
    return strlen (q->out);
}

/* From the shell (bankrun): N names of each form in the tongue, from SEED on */
int __fastcall__ run (int argc, char** argv)
{
    static struct nq q;
    static const char* const forms[3] = { "people: ", "places: ", "peoples: " };
    unsigned char f, i, n;

    if (argc < 2) {
        write (2, "usage: bankrun namegen.bp TONGUE [N [SEED]]\n", 44);
        return 1;
    }
    q.tongue = atoi (argv[1]);
    q.snd = ng_first[(q.tongue ? q.tongue - 1 : 0) % NG_FIRST];
    n = argc > 2 ? atoi (argv[2]) : 6;
    q.seed = argc > 3 ? atoi (argv[3]) : 1;
    for (f = 0; f < 3; ++f) {
        write (1, forms[f], strlen (forms[f]));
        for (i = 0; i < n; ++i) {
            q.form = f;
            name (&q);
            ++q.seed;
            write (1, q.out, strlen (q.out));
            write (1, i + 1 < n ? " " : "\n", 1);
        }
    }
    return 0;
}
