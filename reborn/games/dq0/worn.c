/*
** worn.c - (dq0, overlay 14: sway.c's place_said's) where a thing's to be worn, said (the user's, 2026-11-10: "Wearing
** should extend to rings, necklaces, earrings, and other jewelry, as well as scarves, and should keep track of where
** worn"; HOUSE.md's worn places): "wear the ring on my left hand", "put the scarf on my head", "wear the scarf over my
** face", "put the necklace around my neck", "don the brooch at my left shoulder".  A wear, a don, a put on, or a put
** whose words say a part of the body after "my" ("put X on my <part>": a don there, not a put in a thing) is a don, its
** other the place, in the world's words for it ("@" and the place: harn.h's location as a letter, A HARN_SKULL ... P
** HARN_FOOT; the side said, l or r, else -), the world's things.c's don and part 107's (wworn.c) to judge it
*/

#include <string.h>
#include "dq0.h"
#include "hub.h"

#pragma code-name ("OV14CODE")
#pragma rodata-name ("OV14RODATA")
#pragma local-strings (on)

/* The body's parts a thing's worn on, the words read after "my" (and "left", "right"), each 0-ended; their places, harn.h's
** locations as letters (A the skull, the head and the ears; B the face; C the neck; D the shoulder; G the forearm, the
** wrist; H the hand; I the thorax, the breast; J the abdomen, the waist) */
static const char parts[] = "head\0ears\0ear\0face\0neck\0throat\0shoulder\0breast\0chest\0waist\0hands\0hand\0finger\0"
    "wrist\0";
static const char locs[] = "AAABCCDIIJHHHG";

void worn_said (void)
{
    static const char* p;
    static const char* q;
    static const char* e;
    static unsigned char a, k, n, side;

    a = cmd.action;
    if (!(a == VB_PUT_OBJECT || (a == VB_EQUIP_OBJECT && cmd.aux != IO_HOLD)) || !(p = strstr (cl, " my "))) {
        return;
    }
    q = p + 4;
    side = '-';
    if (!strncmp (q, "left ", 5)) {
        side = 'l';
        q += 5;
    } else if (!strncmp (q, "right ", 6)) {
        side = 'r';
        q += 6;
    }
    for (k = 0, e = parts; *e; ++k, e += n + 1) {
        n = strlen (e);
        if (!strncmp (q, e, n) && (!q[n] || q[n] == ' ')) {
            break;
        }
    }
    if (!*e) {
        return;                                 /* (No part of the body: a put in a thing, "put the ring on my pack") */
    }
    if (!strcmp (cmd.obj.word, e)) {            /* (The place's word taken for the thing's, "wear the scarf over my face":
                                                **   the thing's the word before the place's preposition) */
        for (q = p; q > cl && q[-1] != ' '; --q) {
        }
        for (e = --q; e > cl && e[-1] != ' '; --e) {
        }
        n = q - e;
        if (!n || n > sizeof cmd.obj.word - 1) {
            return;
        }
        memcpy (cmd.obj.word, e, n);
        cmd.obj.word[n] = 0;
        cmd.obj.kind = 0;
    }
    cmd.action = VB_EQUIP_OBJECT;
    if (a == VB_PUT_OBJECT) {
        cmd.aux = IO_DON;                       /* ("Put the scarf on my head": put on, there) */
    }
    cmd.prep = PR_ON;
    cmd.dir = 0xFF;                             /* ("Left": no way) */
    cmd.other.kind = cmd.other.sel = cmd.other.n = 0;
    cmd.other.word[0] = '@';
    cmd.other.word[1] = locs[k];
    cmd.other.word[2] = side;
    cmd.other.word[3] = 0;
}
