/*
** activity.h - (dq0w) what the character is doing, and what's to follow as it's free (ENGINE-CRITIQUE.md's 7: one
** activity per actor, the step planner's 3.3).  Its long task now is the hub's pc_task (and move.on: a walk, a spell
** of digging); the reach's step under way things.h's struct step (rstep, its queue aside); what's queued after the
** long task is here, gathered from the flags that held it, the main loop's free-again chain their order.  The
** denizens' minds post intents the world carries out the same way, and could keep the same record per actor later.
*/

#ifndef ACTIVITY_H
#define ACTIVITY_H

struct pc_activity {
    /* Queued after the long task, popped in this order as the character's free (dq0w.c's act_next): take first,
    ** then each byte from gath on, its own handler's (the main loop's next_[], the same order) */
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
};
#define ACT_NEXT        5                       /* (The bytes gath ... trav, in order: act_next's) */
extern struct pc_activity act;                  /* (dq0w.c's) */

#endif
