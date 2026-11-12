/*
** activity.h - (dq0w) what the character is doing, and what's to follow as it's free (ENGINE-CRITIQUE.md's 7: one
** activity per actor, the step planner's 3.3).  Its long task now is the hub's pc_task (and move.on: a walk, a spell
** of digging); the reach's step under way things.h's struct step (rstep, its queue aside); what's queued after the
** long task is here: each continuation's own field (its thing, its family, its target, its kind of end ...), and a
** stack of them as they were planned (q: act_plan pushes one as its field's set, the last planned on top; dq0w.c's
** act_next, as the character's free, pops those whose field's been cleared since and hands the top to its handler).
** The denizens' minds post intents the world carries out the same way, and could keep the same record per actor
** later.
*/

#ifndef ACTIVITY_H
#define ACTIVITY_H

struct pc_activity {
    /* Queued after the long task (each its kind on q, AK_*, as it's planned: act_plan): */
    unsigned take;                              /* A thing walked over to, to be taken up as it gets there (I_TAKE's
                                                **   item; 1 the square's alone; 0 none: wgame.c's take_up) */
    unsigned char gath;                         /* A gathering of missiles under way (the family, FM_*, | 0x80:
                                                **   wshoot.c's gath_op, GO_ON) */
    unsigned char foe;                          /* A blow's close-in, I_CLOSE's: its target plus 1 (close_on) */
    unsigned char srch;                         /* A search's end: 1 a search; 2 a room searched whole, stop by
                                                **   stop (searched) */
    unsigned char work;                         /* A timed work's end: PA_*, LW_TRACE, LW_HIDE, LW_REACH (a step of
                                                **   the reach: things.c's RO_END); 0 none (work_done) */
    unsigned char trav;                         /* A journey's next leg (1), or a dig's tool put away after it,
                                                **   TRV_TOOL (travel_on) */
    /* The long task's own going on, and its stop */
    unsigned char fight;                        /* Fighting on (part 24's: 1 an enemy may engage it, looked at as it's
                                                **   free; 2 a command took a fight over; | FW_RUN a flight's run on) */
    unsigned char cut;                          /* Its long task stopped, hard or by the player (work.h's: no outcome,
                                                **   the stop told) */
    /* The continuations as they were planned, the last on top (AK_*: each at most once; one whose field's 0 is done,
    ** popped as it comes to the top) */
    unsigned char q[6];
    unsigned char nq;
};
enum { AK_TAKE, AK_GATH, AK_FOE, AK_SRCH, AK_WORK, AK_TRAV };  /* (take; then gath ... trav, the bytes in order) */
extern struct pc_activity act;                  /* (dq0w.c's) */
void __fastcall__ act_plan (unsigned char k);   /* (act.s's: continuation k planned, its field set: on top of the stack,
                                                **   out of its place if it was in it) */

#endif
