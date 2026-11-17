/*
** worn.c - (dq0, overlay 14: sway.c's place_said's) where a thing's to be worn, said (the user's, 2026-11-10: "Wearing
** should extend to rings, necklaces, earrings, and other jewelry, as well as scarves, and should keep track of where
** worn"; HOUSE.md's worn places): "wear the ring on my left hand", "put the scarf on my head", "wear the scarf over my
** face", "put the necklace around my neck", "don the brooch at my left shoulder".  A wear, a don, a put on, or a put
** whose words say a part of the body after "my" (the parser's worn phrase, CF_WORN: its other the place's words, "left
** hand"; "put X on my <part>": a don there, not a put in a thing) is a don, its other the place, in the world's words
** for it ("@" and the place: items.h's place on the body as a letter, A PB_HEAD ... P PB_FOOT; the side said, l or r,
** else -), the world's things.c's don and part 107's (wworn.c) to judge it
*/

#include <string.h>
#include "dq0.h"
#include "hub.h"

#pragma code-name ("OV14CODE")
#pragma rodata-name ("OV14RODATA")
#pragma local-strings (on)

/* The body's parts a thing's worn on, the words read after "my" (and "left", "right"), each 0-ended; their places, items.h's
** PB_* as letters (A the head and the ears; B the face; C the neck; D the shoulder; G the wrist; H the hand; I the
** breast; J the waist) */
static const char parts[] = "head\0ears\0ear\0face\0neck\0throat\0shoulder\0breast\0chest\0waist\0hands\0hand\0finger\0"
    "wrist\0";
static const char locs[] = "AAABCCDIIJHHHG";

void worn_said (void)
{
    static const char* q;
    static const char* e;
    static unsigned char a, k, n, side;

    a = cmd.action;
    if (!(cmd.flags & CF_WORN)) {
        return;
    }
    cmd.flags &= ~CF_WORN;
    if (!(a == VB_PUT_OBJECT || (a == VB_EQUIP_OBJECT && cmd.aux != IO_HOLD))) {
        return;
    }
    q = cmd.other.word;                         /* (Its side the word before its part: "left hand") */
    side = '-';
    if (!strncmp (q, "left ", 5)) {
        side = 'l';
        q += 5;
    } else if (!strncmp (q, "right ", 6)) {
        side = 'r';
        q += 6;
    }
    for (k = 0, e = parts; *e && strcmp (q, e); ++k, e += strlen (e) + 1) {
    }
    if (!*e) {
        return;                                 /* (No part of the body: a put in a thing, "put the ring on my pack") */
    }
    cmd.action = VB_EQUIP_OBJECT;
    if (a == VB_PUT_OBJECT) {
        cmd.aux = IO_DON;                       /* ("Put the scarf on my head": put on, there) */
    }
    cmd.prep = PR_ON;
    cmd.other.kind = cmd.other.sel = cmd.other.n = 0;
    cmd.other.word[0] = '@';
    cmd.other.word[1] = locs[k];
    cmd.other.word[2] = side;
    cmd.other.word[3] = 0;
}
