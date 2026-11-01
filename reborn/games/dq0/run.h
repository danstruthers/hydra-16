/*
** run.h - Dingus Quest Zero: a passage's run, its real heading (the user's, 2026-10-31: ways off the compass, step 2;
** HOUSE.md's a passage's heading): from a walk's steps along it (eighths, from its first), the stretch that keeps one
** shape, and its bearing in sixteenths of a turn (0 north, 1 north-northeast, 2 northeast ... 15 north-northwest).
** Two headings an eighth or a quarter apart, the one that comes twice running the main one, the other only ever one at
** a time (a staircase of short steps, a zigzag): a slant, its bearing its steps' sum, once the other has come twice (the
** two by turns, no main one: halfway between them);
** else the run is straight on its first heading, as far as that goes.  So a bend is a bend (its corner's one step the
** other way, then on that way), and a staircase one slant.  In assembly (run.inc), in the part that wants it (the
** senses' look's, part 2: dq0p/run.s; the world's walk's beats, part 23: dq0w/run.s), with level.h's head_x, head_y.
*/

#define RUN_MAX         8                       /* (The steps looked along: a look's, a walk's beat's) */

extern unsigned char run_s[RUN_MAX];            /* The steps, headings (eighths), from the first */
extern unsigned char run_n;                     /* The run's length, run_of's: its steps from run_s[0] */
unsigned char __fastcall__ run_of (unsigned char n);    /* The run in run_s[0 .. n - 1] (n 1 or more): its bearing
                                                **   in sixteenths, its length in run_n */
