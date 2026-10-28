; ****************************************************************************
; hub.inc - what the command parser (dq0c.s) reads and writes of Dingus Quest Zero's hub (games/dq0/hub.h): bank 0's
; places and the request's, and their constants.  hub.h holds its HUBO_* and CQO_* to the structs (its dq0c_fits),
; and games/dq0c/make.js holds these to hub.h's, so the three agree or the build stops.
; ****************************************************************************

HUB_WINDOW  = $8000                 ; The hub's bank in the window (hub.h's HUB_WINDOW)
HUB_U       = $FFF1                 ; The shared RAM's macro-page register, then the bank register (hub.c's hub_map)
HUB_BANKREG = $00
HUB_MAGIC   = $5144                 ; "DQ" (hub.h's)
HUB_VERSION = 23

HUBO_SEM    = 4                     ; Bank 0: each task's doorbell (a semaphore), by role
HUBO_TASK   = 12                    ;   each role's task number
HUBO_BEAT   = 20                    ;   each role's heartbeat (bumped as it works)
HUBO_CQ     = 28                    ;   the command parser's request (struct cq)
HUBO_QUIT   = 52                    ;   the game's ending (the world's)

RQ_STATE    = 0                     ; A request's header (level.h's RQH): its state, kind, asker and number
RQ_KIND     = 1
RQ_FROM     = 2
RQ_SEQ      = 3
CQO_BANK    = 4                     ; struct cq: the text's bank, the text's bytes (two), the reply's result, the way
CQO_LEN     = 5                     ;   the character faces
CQO_RES     = 7
CQO_FACE    = 8
CQ_WORDS    = 1024                  ; The world's words' text, at most
CQ_CMD      = $1000                 ; A line's reply's struct cmd (hub.h's), in the text bank (cmdmean.s's CM_*)
CMD_SIZE    = 61

REQ_FREE    = 0                     ; hub.h's request states
REQ_POSTED  = 1
REQ_DONE    = 2
CQ_NONE     = 0                     ; hub.h's CQ_*: nothing (the round trip timed), a line matched, the world's words
CQ_MATCH    = 1
CQ_WORLD    = 2
