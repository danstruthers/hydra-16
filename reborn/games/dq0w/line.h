/*
** line.h - (every reader of the chunks) one line between two squares, line.inc's (the world's line.s, the senses'):
** the senses' and the world's (ENGINE-CRITIQUE.md's §4).  A line between the character and anyone is drawn from the
** character's end, its eyes by its body's position, as its look's own lines are (cast.s's): the same squares and
** heights whoever's asking, so a denizen sees the character just when the character would see it (the light aside),
** and the character can shoot what it sees
*/

#define LN_SIGHT        0                       /* The eye's: past glass; */
#define LN_FIRE         1                       /*   a missile's: not glass; */
#define LN_AIR          2                       /*   the air's (a smell's, a sound's portals): not glass, no floor
                                                **   in the way (it fills the place) */
#define LN_CACHED       0x80                    /* (A mode's flag, the senses' alone: a chunk not in the cache in the
                                                **   way, none made for it) */
#define LN_STOP         1                       /* (line's: the line stopped by a square, */
#define LN_WAY          2                       /*   by ln_way's say) */
#define LN_EYE          5                       /* The eyes, feet above the floor (standing; crouched, kneeling, */
#define LN_EYES(pos)    ((pos) ? ((pos) == POS_PRONE ? 1 : 3) : LN_EYE)    /*   sitting three, prone one: the hub's
                                                **   pc_pos, view.c's for the look's) */

unsigned char __fastcall__ ln_portal (const struct portal* o, unsigned char mode);  /* Portal o: 0 it stops the line,
                                                **   1 it passes it, 2 through a narrow opening (a slit, a window) */
unsigned char __fastcall__ ln_pass (unsigned char sn, unsigned x, unsigned y, unsigned char mode);   /* Square x, y:
                                                **   the same (no square, rock or a wall: 0) */
unsigned char __fastcall__ line (unsigned char sn, unsigned x, unsigned y, unsigned tx, unsigned ty,
    unsigned char mode);                        /* From x, y to tx, ty on sn: 0 clear, else LN_STOP, LN_WAY */
extern unsigned char ln_eye;                    /* (line's: the eyes, feet above x, y's floor: LN_EYE, the caller's to
                                                **   set) */
extern unsigned char ln_n, ln_first, ln_last;   /* (line's: the squares it stepped through, the first one's ln_pass and
                                                **   the last one's) */
extern unsigned ln_x, ln_y;                     /* (line's: the square it's at, as ln_way is asked) */
extern unsigned char __fastcall__ (*ln_way) (unsigned char n);  /* (line's: asked of each square (the n-th) the
                                                **   line passes, if set: not 0, the line stopped there, LN_WAY) */
