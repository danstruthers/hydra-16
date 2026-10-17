/*
** eyes.h - (dq0p) the eyes: eyes.c
*/

struct eyes {                                   /* An actor's eyes (the senses'): */
    unsigned char   a;                          /*   the light they're used to, in 16ths of a level (0 the dark), */
    unsigned long   t;                          /*   as of then (0: never yet), */
    unsigned long   dz;                         /*   dazzled till then */
};
extern struct eyes E[1 + NACTORS];              /* (The character's 0, an actor's its entry plus 1) */
struct glows {                                  /* (struct sight's, from its nglows: copied in) */
    unsigned char   nglows;
    struct { unsigned char rel, dist, how; } glow[SEE_GLOWS];
    unsigned char   dazzled;
};
extern struct glows G;
void __fastcall__ adapt (struct eyes* e, unsigned char lv, unsigned char rate);    /* As time's gone, in light lv */
unsigned char __fastcall__ glow_reach (const struct eyes* e, unsigned char b);   /* How far a light of brightness b
                                                **   is seen far off */
unsigned char __fastcall__ clear_line (unsigned char sn, unsigned x, unsigned y, int dx, int dy, unsigned dist,
    unsigned char mk);                          /* No rock, wall nor shut door between (mk: chunks made for it) */
void __fastcall__ glows (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char range,
    unsigned char light);                       /* The character's eyes adapted, the lights it sees far off (G) */
