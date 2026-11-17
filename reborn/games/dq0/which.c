/*
** which.c - (overlay 24) the one "which?" (COMMANDS-CRITIQUE.md's 5.1, step D, 2026-11-07): the resolver's question asked
** as the world's is, by the designators' capitals (o_which_seen: resolve.c's ask_which), and an answer read by where a
** choice lies (o_which_way: letters.c's which_in, which reads the rest, a capital, its number, a word of one's alone, and
** makes the command again on the chosen one's capital).  The resolver's own question and answer (its "Which do you
** mean: 1. ...", its kept command) gone with it.
*/

#pragma code-name ("OV24CODE")
#pragma rodata-name ("OV24RODATA")
#pragma data-name ("OV24BSS")
#pragma bss-name ("OV24BSS")
#pragma local-strings (on)

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

#define WSIZE       sizeof UIREC->which_w[0]
#define WAT(b, k)   (*(unsigned*) ((b) + (k)))  /* (A choice's square: its x at 15, its y at 17) */

static char o[WSIZE];                           /* (A choice, the UI bank's which_w[k], copied in) */
static const char* const rel[8] = {             /* (The ways, as the character faces) */
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};

/* The UI bank's bytes copied into this bank (from_ui), or this bank's out to it (to_ui), by way of the C stack (the two
** share the window); n at most WSIZE */
#pragma static-locals (push, off)
static void __fastcall__ from_ui (void* to, const void* from, unsigned char n)
{
    char t[WSIZE];

    hub_mem (UI_BANK, t, from, n);
    memcpy (to, t, n);
}

static void __fastcall__ to_ui (void* to, const void* from, unsigned char n)
{
    char t[WSIZE];

    memcpy (t, from, n);
    hub_mem (UI_BANK, to, t, n);
}

/* The store's thing n: its id and its kind (the store's record, by way of the C stack) */
static unsigned __fastcall__ id_of (unsigned n, unsigned char* kind)
{
    struct item it;

    hub_mem (ITEM_BANK (n), &it, ITEM_AT (n), sizeof it);
    *kind = it.kind;
    return it.id;
}
#pragma static-locals (pop)
#pragma static-locals (on)                      /* (The rest's locals static, in this bank: the code the less) */

/* Squares from the character to (x, y) (the more of the two ways: resolve.c's apart) */
static unsigned char __fastcall__ apart (unsigned x, unsigned y)
{
    x -= sight_copy.x;
    y -= sight_copy.y;
    x = (int) x < 0 ? -x : x;
    y = (int) y < 0 ? -y : y;
    x = x > y ? x : y;
    return x > 255 ? 255 : x;
}

/* Capital j's text at b ("(B) "; "" none: caps.c's cap_text) */
static void __fastcall__ cap_at (char* b, unsigned char j)
{
    if (j >= NLET) {
        return;
    }
    *b++ = '(';
    if (j >= NLET / 2) {
        *b++ = 'A';
        j -= NLET / 2;
    }
    *b++ = 'A' + j;
    strcpy (b, ") ");
}

/* The resolver's question (resolve.c's ask_which: of several things or denizens it sees, the one meant), asked as the
** world's is, by the designators: each choice (the UI bank's which_w[k], n of them, the resolver's: its name, its square
** at 15, its way at 19 (0xFF where the character stands), a denizen's 1 at 20, its place in the sight record at 21)
** given its capital (at 23: its own, by that place, so several on one square each have theirs, not the last's, the
** user's three cave adders all (G), 2026-11-17) and told where it lies: "Which one? (A) the giant rat ahead to your
** left, on the right, (B) the giant rat ahead to your left, on the left, (C) the giant rat ahead" (on one square, no
** side said); the answer letters.c's which_in's, the command made again on that one's capital (the thing of role r's
** word: its other's, 1; none said, "take", the capital after it).  which_n | WN_SEEN */
void __fastcall__ o_which_seen (unsigned char r, unsigned char n)
{
    static unsigned wx[4], wy[4];
    static unsigned char wh[4], k, j, kd;
    static int sk, sj;
    static char b[8];

    for (k = 0; k < n; ++k) {
        from_ui (o, UIREC->which_w[k], WSIZE);
        wx[k] = WAT (o, 15);
        wy[k] = WAT (o, 17);
        wh[k] = o[19];
        j = o[21];
        o[WSIZE - 1] = o[20] ? cap_index (CAP_DEN (sight_copy.den[j].den), CW_DEN, 0) :
            cap_index (id_of (sight_copy.item[j].n, &kd), CW_THING, kd);
        to_ui (UIREC->which_w[k], o, WSIZE);
    }
    strcpy (lb, "Which one?");
    for (k = 0; k < n; ++k) {
        from_ui (o, UIREC->which_w[k], WSIZE);
        strcat (lb, k ? ", " : " ");
        cap_at (lb + strlen (lb), o[WSIZE - 1]);
        strcat (strcat (strcat (strcat (lb, "the "), o), " "), wh[k] == 0xFF ? "where you stand" : rel[(wh[k] - pc_face) & 7]);
        for (j = 0; j < n; ++j) {               /* (Another the same way: how far off each is; as far, which side) */
            if (j != k && wh[j] == wh[k] && wh[k] != 0xFF) {
                if (apart (wx[j], wy[j]) == apart (wx[k], wy[k])) {
                    sk = head_x[pc_face] * ((int) wy[k] - sight_copy.y) - head_y[pc_face] * ((int) wx[k] - sight_copy.x);
                    sj = head_x[pc_face] * ((int) wy[j] - sight_copy.y) - head_y[pc_face] * ((int) wx[j] - sight_copy.x);
                    if (sk != sj) {
                        strcat (lb, sk < sj ? ", on the left" : ", on the right");
                    }
                } else {
                    strcat (strcat (strcat (lb, ", "), utoa (5 * apart (wx[k], wy[k]), b, 10)), " feet off");
                }
                break;
            }
        }
    }
    hub_mem (UI_BANK, UIREC->which_l, cl, strlen (cl) + 1);   /* (cl and cmd: the main RAM's, copied straight) */
    hub_mem (UI_BANK, UIREC->which_o, r ? cmd.other.word : cmd.obj.word, sizeof UIREC->which_o);
    k = n | WN_SEEN;
    to_ui (&UIREC->which_n, &k, 1);
    which_on = 1;
    msg (lb);
}

/* The resolver's question's answer by where its choice lies (word p, len long; n choices, their squares and ways in the UI
** bank's which_w: o_which_seen's): a way as the character faces ("ahead", "right", "behind", "left") or of the compass
** ("north" ...), the nearest that way; "closest", "nearest", the nearest of them all.  Its place, 0xFE none that way,
** 0xFF not a way (resolve.c's answered's till 2026-11-07) */
unsigned char __fastcall__ o_which_way (const char* p, unsigned char len, unsigned char n)
{
    static const char rw[4][7] = { "ahead", "right", "behind", "left" };
    static unsigned char h, i, k, d, bd;

    h = 0xFF;
    for (i = 0; i < 4; ++i) {
        if (strlen (rw[i]) == len && !strncmp (p, rw[i], len)) {
            h = (2 * i + pc_face) & 7;
        }
    }
    for (i = 0; i < 8; ++i) {
        if (strlen (head_name[i]) == len && !strncmp (p, head_name[i], len)) {
            h = i;
        }
    }
    if (h == 0xFF && (len != 7 || (strncmp (p, "closest", 7) && strncmp (p, "nearest", 7)))) {
        return 0xFF;
    }
    for (k = 0xFE, bd = 0xFF, i = 0; i < n; ++i) {
        from_ui (o, UIREC->which_w[i], WSIZE);
        if ((h == 0xFF || (unsigned char) o[19] == h) && (d = apart (WAT (o, 15), WAT (o, 17))) < bd) {
            bd = d;
            k = i;
        }
    }
    return k;
}

/* choose.c's question of doors and openings (o_pick's, several of a kind and none ahead: "Which door? (1) the door ahead
** to your left (closed), (2) the one to your right (open)"), asked in lb by choose.c, its choices put here as it words
** them (o_which_put: choice k's words w, a few to answer it by, "door closed"; its square, its way) and the question
** then made the one which_in answers (o_which_place: n of them; the line kept, cl, made again as it was, the chosen
** square main.c's pick_x and pick_y, which o_pick takes).  which_n | WN_PLACE (2026-11-07, step E: choose.c's own
** answer, its kept command, gone) */
void __fastcall__ o_which_put (unsigned char k, const char* w, unsigned x, unsigned y, unsigned char h)
{
    memset (o, 0, WSIZE);
    strncpy (o, w, 14);
    WAT (o, 15) = x;
    WAT (o, 17) = y;
    o[19] = h;
    o[WSIZE - 1] = NLET;
    to_ui (UIREC->which_w[k], o, WSIZE);
}

void __fastcall__ o_which_place (unsigned char n)
{
    static unsigned char k;

    hub_mem (UI_BANK, UIREC->which_l, cl, strlen (cl) + 1);
    k = 0;
    to_ui (UIREC->which_o, &k, 1);
    k = n | WN_PLACE;
    to_ui (&UIREC->which_n, &k, 1);
    which_on = 1;
    msg (lb);
}
